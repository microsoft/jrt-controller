// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
package jbpf

// LoadCodeletSetRequestSerde is a model contains serde information
type LoadCodeletSetRequestSerde struct {
	FilePath string `json:"file_path"`
}

// LoadCodeletSetRequestIOMap is a model contains IO map information
type LoadCodeletSetRequestIOMap struct {
	Name     string                      `json:"name"`
	StreamID string                      `json:"stream_id"`
	Serde    *LoadCodeletSetRequestSerde `json:"serde,omitempty"`
}

// LoadCodeletSetRequestLinkedMap is a model contains linked map information
type LoadCodeletSetRequestLinkedMap struct {
	LinkedCodeletName string `json:"linked_codelet_name"`
	LinkedMapName     string `json:"linked_map_name"`
	MapName           string `json:"map_name"`
}

// LoadCodeletSetRequestDescriptors is a model contains codelet descriptor information
type LoadCodeletSetRequestDescriptors struct {
	CodeletPath      string                            `json:"codelet_path"`
	HookName         string                            `json:"hook_name"`
	InIOChannel      []*LoadCodeletSetRequestIOMap     `json:"in_io_channel,omitempty"`
	LinkedMaps       []*LoadCodeletSetRequestLinkedMap `json:"linked_maps,omitempty"`
	CodeletName      string                            `json:"codelet_name"`
	OutIOChannel     []*LoadCodeletSetRequestIOMap     `json:"out_io_channel,omitempty"`
	Priority         *uint32                           `json:"priority,omitempty"`
	RuntimeThreshold *uint32                           `json:"runtime_threshold,omitempty"`
}

// LoadCodeletSetRequest is a request to load a codelet set
type LoadCodeletSetRequest struct {
	CodeletDescriptors []*LoadCodeletSetRequestDescriptors `json:"codelet_descriptor"`
	CodeletSetID       string                              `json:"codeletset_id"`
}
