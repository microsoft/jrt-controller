// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
package common

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/sirupsen/logrus"
)

// FileData represents a file
type FileData struct {
	AbsPath  string
	Content  []byte
	FileName string
}

// LoadFilesByGlob loads files by glob
func LoadFilesByGlob(logger *logrus.Logger, glob string) (map[string]*FileData, error) {
	matches, err := filepath.Glob(glob)
	if err != nil {
		return nil, err
	}
	logger.WithFields(logrus.Fields{
		"glob":    glob,
		"matches": strings.Join(matches, ", "),
	}).Debug("loading files by glob")
	files := make(map[string]*FileData)
	for _, m := range matches {
		content, err := readFile(logger, m)
		if err != nil {
			return nil, err
		}
		files[filepath.Base(m)] = &FileData{
			AbsPath:  m,
			FileName: filepath.Base(m),
			Content:  content,
		}
	}

	return files, nil
}

func readFile(logger *logrus.Logger, elem ...string) ([]byte, error) {
	filePath := filepath.Join(elem...)
	l := logger.WithField("filepath", filePath)
	l.Debug("reading file")
	fi, err := os.Stat(filePath)
	if err != nil {
		l.WithError(err).Errorf("failed to stat file")
		return nil, err
	} else if fi.IsDir() {
		err := fmt.Errorf(`expected "%s" to be a file, got directory`, filePath)
		l.WithError(err).Errorf("path is invalid")
		return nil, err
	}
	out, err := os.ReadFile(filePath)
	if err != nil {
		l.WithError(err).Errorf("failed to read file")
		return nil, err
	}
	l.WithField("length", len(out)).Debugf("successfully read file")
	return out, nil
}
