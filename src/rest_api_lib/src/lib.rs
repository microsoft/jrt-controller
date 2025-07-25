// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
use std::ffi::CString;
use axum::{
    extract,
    extract::{Path, State},
    http::StatusCode,
    response::IntoResponse,
    routing::get,
    Json, Router,
};
use axum_server::Handle;
use chrono::prelude::{DateTime, Utc};
use serde::{Deserialize, Serialize};
use std::net::SocketAddr;
use std::os::raw::{c_char, c_int, c_void};
use std::sync::mpsc::channel;
use std::sync::Arc;
use std::thread;
use std::time::{Duration, SystemTime};
use tokio::runtime::Runtime;
use tokio::sync::Mutex;
use utoipa::{OpenApi, ToSchema};
use utoipa_swagger_ui::SwaggerUi;
use std::collections::HashMap;

#[repr(C)]
pub struct KeyValuePair {
    pub key: *mut i8,
    pub val: *mut i8,
}

#[repr(C)]
pub struct LoadAppRequest {
    pub app: *mut i8,
    pub app_size: usize,
    pub app_name: *mut c_char,
    pub runtime_us: u32,
    pub deadline_us: u32,
    pub period_us: u32,
    pub ioq_size: u32,
    pub app_path: *mut c_char,
    pub app_type: *mut c_char,
    pub app_params: [KeyValuePair; 255], // Fixed-size array
    pub device_mapping: [KeyValuePair; 255], // Fixed-size array
    pub app_modules: [*mut c_char; 255], // Fixed-size array
}

type LoadAppCallback = unsafe extern "C" fn(load_req: LoadAppRequest) -> c_int;
type UnloadAppCallback = unsafe extern "C" fn(app_id: c_int) -> c_int;

#[derive(Clone)]
pub struct Callbacks {
    load_app: Option<LoadAppCallback>,
    unload_app: Option<UnloadAppCallback>,
}

type Store = Mutex<Vec<JrtcAppState>>;
#[derive(Clone)]
pub struct ServerState {
    store: Arc<Store>,
    callbacks: Callbacks,
}

#[derive(Serialize, Deserialize, ToSchema, Clone)]
struct JrtcAppLoadRequest {
    #[serde(with = "base64_bytes")]
    app: Vec<u8>,
    app_name: String,
    runtime_us: u32,
    deadline_us: u32,
    period_us: u32,
    ioq_size: u32,
    app_path: String,
    app_type: String,
    app_params: HashMap<String, String>,
    device_mapping: HashMap<String, String>,
    app_modules: Vec<String>,
}

#[derive(Serialize, Deserialize, ToSchema, Clone)]
struct JrtcAppState {
    id: i32,
    request: JrtcAppLoadRequest,
    start_time: String,
}

#[derive(Serialize, Deserialize, ToSchema)]
enum JrtcAppError {
    /// jrt-controller app not found by id.
    #[schema(example = "error message")]
    Details(String),
}

#[tokio::main(flavor = "current_thread")]
async fn spawn_server(handle: Handle, port: u16, callbacks: Callbacks) {
    let state = ServerState {
        callbacks,
        store: Arc::new(Store::default()),
    };

    #[derive(OpenApi)]
    #[openapi(
        info(description = "jrt-controller control plane REST API", title = "jrt-controller REST API"),
        paths(get_app, get_apps, load_app, unload_app,),
        tags(
            (name = "app", description = "jrt-controller application API")
        ),
        components(schemas(JrtcAppLoadRequest, JrtcAppState, JrtcAppError))
    )]
    struct ApiDoc;

    // build our application with a route
    let app = Router::new()
        .merge(SwaggerUi::new("/swagger-ui").url("/api-docs/openapi.json", ApiDoc::openapi()))
        .route("/app", get(get_apps).post(load_app))
        .route("/app/:id", get(get_app).delete(unload_app))
        .with_state(state);

    let addr = SocketAddr::from(([0, 0, 0, 0], port));
    println!("listening on {}", addr);
    axum_server::bind(addr)
        .handle(handle)
        .serve(app.into_make_service())
        .await
        .unwrap();
}

#[utoipa::path(
    get,
    path = "/app/{id}",
    tag = "app",
    responses(
        (status = 200, description = "Successfully fetched jrtc application state", body = JrtcAppState),
        (status = 404, description = "jrt-controller application not found", body = JrtcAppError, example = json!(JrtcAppError::Details(String::from("id = 1"))))
    ),
    params(
        ("id" = i32, Path, description = "jrt-controller application id")
    )
  )]
async fn get_app(Path(id): Path<i32>, State(state): State<ServerState>) -> impl IntoResponse {
    let mut apps = state.store.lock().await;

    let app = apps.iter_mut().find(|app| app.id == id);
    if let Some(x) = app {
        (StatusCode::OK, Json(x)).into_response()
    } else {
        (
            StatusCode::NOT_FOUND,
            Json(JrtcAppError::Details(format!("id = {}", id))),
        )
            .into_response()
    }
}

#[utoipa::path(
    get,
    path = "/app",
    tag = "app",
    responses(
        (status = 200, description = "Successfully fetched jrtc application states", body = [JrtcAppState])
    )
  )]
async fn get_apps(State(state): State<ServerState>) -> impl IntoResponse {
    let apps = state.store.lock().await;
    (StatusCode::OK, Json(apps.as_slice())).into_response()
}

#[utoipa::path(
    post,
    path = "/app",
    tag = "app",
    request_body = JrtcAppLoadRequest,
    responses(
        (status = 200, description = "Successfully loaded jrtc application", body = JrtcAppState),
        (status = 400, description = "Bad request", body = JrtcAppError, example = json!(JrtcAppError::Details(String::from("Bad request")))),
        (status = 500, description = "Internal error", body = JrtcAppError, example = json!(JrtcAppError::Details(String::from("Internal server error"))))
    )
  )]
async fn load_app(
    State(state): State<ServerState>,
    extract::Json(payload): extract::Json<JrtcAppLoadRequest>,
) -> impl IntoResponse {
    let payload_cloned = payload.clone();
    let app_name = payload_cloned.app_name;
    let app_path = payload_cloned.app_path;
    let app_type = payload_cloned.app_type;
    let app_params = payload_cloned.app_params;
    let device_mapping = payload_cloned.device_mapping;
    let app_modules = payload_cloned.app_modules;

    let c_app_name = match CString::new(app_name.clone()) {
        Ok(c) => c,
        Err(_) => {
            return (
                StatusCode::BAD_REQUEST,
                Json(JrtcAppError::Details(format!(
                    "app_name cannot be converted into c string = {}",
                    app_name.clone()
                ))),
            )
            .into_response();
        }
    };

    let c_app_path = match CString::new(app_path.clone()) {
        Ok(c) => c,
        Err(_) => {
            return (
                StatusCode::BAD_REQUEST,
                Json(JrtcAppError::Details(format!(
                    "app_path cannot be converted into c string = {}",
                    app_path.clone()
                ))),
            )
            .into_response();
        }
    };

    let c_app_type = match CString::new(app_type.clone()) {
        Ok(c) => c,
        Err(_) => {
            return (
                StatusCode::BAD_REQUEST,
                Json(JrtcAppError::Details(format!(
                    "app_type cannot be converted into c string = {}",
                    app_type.clone()
                ))),
            )
            .into_response();
        }
    };

    let mut c_app_params: [KeyValuePair; 255] = unsafe { std::mem::zeroed() }; // Initialize
    let mut c_device_mapping: [KeyValuePair; 255] = unsafe { std::mem::zeroed() }; // Initialize

    for (i, (key, value)) in app_params.iter().enumerate() {
        if i >= 255 {
            break; // Prevent overflow
        }
        let c_key = CString::new(key.clone()).unwrap().into_raw();
        let c_val = CString::new(value.clone()).unwrap().into_raw();

        c_app_params[i] = KeyValuePair { key: c_key, val: c_val };
    }

    for (i, (key, value)) in device_mapping.iter().enumerate() {
        if i >= 255 {
            break; // Prevent overflow
        }
        let c_key = CString::new(key.clone()).unwrap().into_raw();
        let c_val = CString::new(value.clone()).unwrap().into_raw();

        c_device_mapping[i] = KeyValuePair { key: c_key, val: c_val };
    }

    let mut c_app_modules: [*mut c_char; 255] = unsafe { std::mem::zeroed() }; // Initialize
    for (i, module) in app_modules.iter().enumerate() {
        if i >= 255 {
            break; // Prevent overflow
        }
        let c_module = CString::new(module.clone()).unwrap().into_raw();
        c_app_modules[i] = c_module;
    }

    let app_req = LoadAppRequest {
        app: payload.app.as_ptr() as *mut i8,
        app_size: payload.app.len(),
        app_name: c_app_name.into_raw(),
        runtime_us: payload.runtime_us,
        deadline_us: payload.deadline_us,
        period_us: payload.period_us,
        ioq_size: payload.ioq_size,
        app_path: c_app_path.into_raw(),
        app_type: c_app_type.into_raw(),
        app_params: c_app_params,
        device_mapping: c_device_mapping,
        app_modules: c_app_modules,
    };

    let response: i32;
    let app_name_ptr = app_req.app_name;
    let app_path_ptr = app_req.app_path;
    let app_type_ptr = app_req.app_type;

    unsafe {
        response = match state.callbacks.load_app {
            Some(load_app_cbk) => load_app_cbk(app_req),
            None => {
                eprintln!("load_app callback not set");
                return (
                    StatusCode::INTERNAL_SERVER_ERROR,
                    Json(JrtcAppError::Details(String::from(
                        "load_app callback is not set",
                    ))),
                )
                .into_response();
            }
        };

        // Free app_name memory after callback
        let _ = CString::from_raw(app_name_ptr);
        let _ = CString::from_raw(app_path_ptr);
        let _ = CString::from_raw(app_type_ptr);
    }

    match response {
        -1 => (
            StatusCode::BAD_REQUEST,
            Json(JrtcAppError::Details(String::from("Bad request"))),
        )
            .into_response(),
        i32::MIN..=-2 => (
            StatusCode::INTERNAL_SERVER_ERROR,
            Json(JrtcAppError::Details(String::from(
                "Internal server error",
            ))),
        )
            .into_response(),
        _ => {
            let mut apps = state.store.lock().await;

            let dt: DateTime<Utc> = SystemTime::now().clone().into();
            let app = JrtcAppState {
                id: response,
                request: payload,
                start_time: format!("{}", dt.format("%+")),
            };

            apps.push(app.clone());

            (StatusCode::OK, Json(app)).into_response()
        }
    }
}

#[utoipa::path(
  delete,
  path = "/app/{id}",
  tag = "app",
  responses(
    (status = 204, description = "Successfully deleted jrtc application if it existed"),
    (status = 500, description = "Internal error", body = JrtcAppError, example = json!(JrtcAppError::Details(String::from("Internal server error"))))
  ),
  params(
      ("id" = i32, Path, description = "jrt-controller application id")
  )
)]
pub async fn unload_app(
    Path(id): Path<i32>,
    State(state): State<ServerState>,
) -> impl IntoResponse {
    // Call the unload callback
    let response: i32 = unsafe {
        match state.callbacks.unload_app {
            Some(unload_app_cbk) => unload_app_cbk(id),
            None => return (StatusCode::INTERNAL_SERVER_ERROR).into_response(),
        }
    };

    // Handle the callback response
    if response == -1 {
        return StatusCode::NO_CONTENT.into_response();
    } else if response <= -2 {
        return (
            StatusCode::INTERNAL_SERVER_ERROR,
            Json(JrtcAppError::Details("Internal server error".to_string())),
        )
            .into_response();
    }

    // Safely remove the app from the store
    let mut apps = state.store.lock().await;
    if let Some(index) = apps.iter().position(|x| x.id == id) {
        apps.remove(index);
        StatusCode::NO_CONTENT.into_response()
    } else {
        (
            StatusCode::NOT_FOUND,
            Json(JrtcAppError::Details("App not found".to_string())),
        )
            .into_response()
    }
}

#[no_mangle]
pub extern "C" fn jrtc_create_rest_server() -> *mut c_void {
    let handle = Arc::new(Handle::new());
    Arc::into_raw(handle) as *mut c_void
}

async fn stop_server(handle: Arc<Handle>) {
    handle.graceful_shutdown(Some(Duration::from_secs(0)));
    println!("Shutdown signal sent");
}

#[no_mangle]
pub extern "C" fn jrtc_stop_rest_server(ptr: *mut c_void) {
    assert!(!ptr.is_null(), "Null pointer passed to jrtc_stop_rest_server");

    // Convert raw pointer back to Arc
    let handle = unsafe { Arc::from_raw(ptr as *mut Handle) };

    let (tx, rx) = channel();
    println!("Sending graceful shutdown signal");

    // Spawn a thread to handle the shutdown
    thread::spawn(move || {
        // Create a single Tokio runtime
        let rt = Runtime::new().unwrap();

        rt.block_on(async {
            stop_server(handle.clone()).await; // Use `Arc` to share safely
            tx.send(()).unwrap(); // Signal completion
        });

        // Drop the Arc reference inside the thread
        drop(handle);
    });

    // Wait for the shutdown to complete
    rx.recv().unwrap();
}

#[no_mangle]
pub extern "C" fn jrtc_start_rest_server(ptr: *mut c_void, port: u16, cbs: *mut Callbacks) {
    println!("Starting REST server at port {}", port);

    // Check for null pointers and handle errors
    if ptr.is_null() || cbs.is_null() {
        eprintln!("Invalid server handle or callbacks pointer");
        return;
    }

    // Safe usage of raw pointers with proper memory management
    unsafe {
        let handle = Arc::from_raw(ptr as *mut Handle); // Use Arc for safe shared ownership
        let callbacks = (*cbs).clone(); // Clone callbacks to ensure ownership
        spawn_server((*handle).clone(), port, callbacks);

        // Convert back into raw pointer to avoid premature drop
        let _ = Arc::into_raw(handle);
    }
}
