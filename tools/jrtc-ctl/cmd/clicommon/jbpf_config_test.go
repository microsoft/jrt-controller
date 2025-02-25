// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package clicommon

import (
	"fmt"
	"os"
	"testing"

	_ "embed"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestUnmarshalConfig(t *testing.T) {
	// Arrange - create yaml file with valid config
	fakeCodeletFile1, cleanup1, err := createTempFileWithContent("codelet*.o", "")
	defer cleanup1()
	require.Nil(t, err)
	fakeCodeletFile2, cleanup2, err := createTempFileWithContent("codelet*.o", "")
	defer cleanup2()
	require.Nil(t, err)
	_, cleanup3, err := createFileWithContent("/tmp/proto_pkg.pb", "")
	defer cleanup3()
	require.Nil(t, err)
	_, cleanup4, err := createFileWithContent("/tmp/proto_pkg:msg_name_serializer.so", "")
	defer cleanup4()
	require.Nil(t, err)
	require.Nil(t, os.Setenv("JBPF_CODELET_PATH", "/tmp"))
	filename, cleanup5, err := createTempFileWithContent("example*.yaml", fmt.Sprintf(`codeletset_id: unique_id_for_codelet_set_1
codelet_descriptor:
- codelet_name: codelet1
  codelet_path: %s
  hook_name: test1
  priority: 2
- codelet_name: codelet2
  codelet_path: %s
  hook_name: test2
  runtime_threshold: 50000
  linked_maps:
    - map_name: p2_counter
      linked_codelet_name: codelet1
      linked_map_name: p1_counter
  out_io_channel:
    - name: map1
      serde:
        file_path: ${JBPF_CODELET_PATH}/proto_pkg:msg_name_serializer.so
        protobuf:
          package_path: ${JBPF_CODELET_PATH}/proto_pkg.pb
          msg_name: msg_name
`, fakeCodeletFile1, fakeCodeletFile2))
	defer cleanup5()
	require.Nil(t, err)

	// Act - parse the yaml file
	cfg, err := JBPFConfigFromYaml(filename)
	require.Nil(t, err)

	// Assert - check the parsed config
	assert.Equal(t, "unique_id_for_codelet_set_1", cfg.ID)
	require.Len(t, cfg.Codelets, 2)

	codelet1 := cfg.Codelets[0]
	assert.Equal(t, fakeCodeletFile1, codelet1.CodeletPath)
	assert.Equal(t, "test1", codelet1.HookName)
	assert.Equal(t, uint32(2), *codelet1.Priority)
	assert.Equal(t, (*uint32)(nil), codelet1.RuntimeThreshold)

	codelet2 := cfg.Codelets[1]
	assert.Equal(t, fakeCodeletFile2, codelet2.CodeletPath)
	assert.Equal(t, "test2", codelet2.HookName)
	assert.Equal(t, (*uint32)(nil), codelet2.Priority)
	assert.Equal(t, uint32(50000), *codelet2.RuntimeThreshold)

	require.Len(t, codelet2.OutIOChannel, 1)
	outMap := codelet2.OutIOChannel[0]
	assert.Equal(t, "map1", outMap.Name)
	assert.Equal(t, "/tmp/proto_pkg.pb", outMap.Serde.Protobuf.PackagePath)
	assert.Equal(t, "${JBPF_CODELET_PATH}/proto_pkg:msg_name_serializer.so", outMap.Serde.FilePath)
	assert.Equal(t, "msg_name", outMap.Serde.Protobuf.MessageName)

	require.Len(t, codelet2.LinkedMaps, 1)
	linkedMap := codelet2.LinkedMaps[0]
	assert.Equal(t, "p2_counter", linkedMap.MapName)
	assert.Equal(t, "codelet1", linkedMap.LinkedCodeletName)
	assert.Equal(t, "p1_counter", linkedMap.LinkedMapName)
}

func TestWithOutOfOrderCodeletFails(t *testing.T) {
	// Arrange - create yaml file with valid config
	fakeCodeletFile1, cleanup1, err := createTempFileWithContent("codelet*.o", "")
	defer cleanup1()
	require.Nil(t, err)
	fakeCodeletFile2, cleanup2, err := createTempFileWithContent("codelet*.o", "")
	defer cleanup2()
	require.Nil(t, err)
	filename, cleanup, err := createTempFileWithContent("example*.yaml", fmt.Sprintf(`codeletset_id: unique_id_for_codelet_set_1
codelet_descriptor:
- codelet_name: codelet2
  codelet_path: %s
  hook_name: test2
  linked_maps:
    - map_name: p2_counter
      linked_codelet_name: codelet1
      linked_map_name: p1_counter
- codelet_name: codelet1
  codelet_path: %s
  hook_name: test1
`, fakeCodeletFile1, fakeCodeletFile2))
	defer cleanup()
	require.Nil(t, err)

	// Act - parse the yaml file
	cfg, err := JBPFConfigFromYaml(filename)

	// Assert - check the parsed config
	require.NotNil(t, err)
	require.Nil(t, cfg)
	assert.Equal(t, fmt.Sprint(err), "linked map codelet1 for codelet codelet2 referenced before its loaded")
}

func TestWithLinkedMapsLoopFails(t *testing.T) {
	// Arrange - create yaml file with valid config
	fakeCodeletFile1, cleanup1, err := createTempFileWithContent("codelet*.o", "")
	defer cleanup1()
	require.Nil(t, err)
	fakeCodeletFile2, cleanup2, err := createTempFileWithContent("codelet*.o", "")
	defer cleanup2()
	require.Nil(t, err)
	filename, cleanup, err := createTempFileWithContent("example*.yaml", fmt.Sprintf(`codeletset_id: unique_id_for_codelet_set_1
codelet_descriptor:
- codelet_name: codelet1
  codelet_path: %s
  hook_name: test1
  linked_maps:
    - map_name: p2_counter
      linked_codelet_name: codelet2
      linked_map_name: p2_counter
- codelet_name: codelet2
  codelet_path: %s
  hook_name: test2
  linked_maps:
    - map_name: p2_counter
      linked_codelet_name: codelet1
      linked_map_name: p1_counter
`, fakeCodeletFile1, fakeCodeletFile2))
	defer cleanup()
	require.Nil(t, err)

	// Act - parse the yaml file
	cfg, err := JBPFConfigFromYaml(filename)

	// Assert - check the parsed config
	require.NotNil(t, err)
	assert.Nil(t, cfg)
	assert.Equal(t, fmt.Sprint(err), "linked map codelet2 for codelet codelet1 referenced before its loaded")
}

func createTempFileWithContent(pattern, content string) (string, func(), error) {
	f, err := os.CreateTemp("", pattern)
	if err != nil {
		return "", nil, err
	}
	cleanup := func() {
		if err := os.Remove(f.Name()); err != nil {
			fmt.Printf("failed to remove temp file: %v\n", err)
		}
	}

	if err := os.Chmod(f.Name(), 0644); err != nil {
		cleanup()
		return "", nil, err
	}

	_, err = f.Write([]byte(content))
	if err != nil {
		cleanup()
		return "", nil, err
	}

	if err := f.Close(); err != nil {
		cleanup()
		return "", nil, err
	}

	return f.Name(), cleanup, nil
}

func createFileWithContent(name, content string) (string, func(), error) {
	f, err := os.Create(name)
	if err != nil {
		return "", nil, err
	}
	cleanup := func() {
		if err := os.Remove(f.Name()); err != nil {
			fmt.Printf("failed to remove temp file: %v\n", err)
		}
	}

	if err := os.Chmod(f.Name(), 0644); err != nil {
		cleanup()
		return "", nil, err
	}

	_, err = f.Write([]byte(content))
	if err != nil {
		cleanup()
		return "", nil, err
	}

	if err := f.Close(); err != nil {
		cleanup()
		return "", nil, err
	}

	return f.Name(), cleanup, nil
}
