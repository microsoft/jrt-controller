package loganalytics

import (
	"bytes"
	"context"
	"errors"
	"sync"
	"time"

	"github.com/sirupsen/logrus"
)

var (
	delimiter = []byte(",")
	prefix    = []byte("[")
	suffix    = []byte("]")
)

// Uploader is a log analytics uploader client
type Uploader struct {
	activeRequests sync.WaitGroup
	bs             bytes.Buffer
	client         *client
	logger         *logrus.Logger
	m              sync.Mutex
	opts           *UploaderOptions
	timer          *time.Timer
}

// NewUploader creates a new log analytics uploader client
func NewUploader(ctx context.Context, logger *logrus.Logger, opts *UploaderOptions) (*Uploader, error) {
	if !opts.enabled {
		return nil, nil
	}

	client, err := newClient(ctx, logger, opts.client)
	if err != nil {
		return nil, err
	}

	u := &Uploader{
		bs:     bytes.Buffer{},
		client: client,
		logger: logger,
		opts:   opts,
		timer:  time.NewTimer(opts.flushPeriod),
	}

	go func() {
		for {
			select {
			case <-ctx.Done():
				return
			case <-u.timer.C:
				if err := u.flush(false); err != nil {
					logger.WithError(err).Error("Failed to flush")
				}
			}
		}
	}()

	return u, nil
}

// Close gracefully shuts down the client
func (u *Uploader) Close() error {
	if err := u.flush(false); err != nil {
		return err
	}

	u.timer.Stop()
	u.activeRequests.Wait()
	return nil
}

// Upload adds data to a buffer to send to a log analytics
func (u *Uploader) Upload(bs []byte) error {
	u.m.Lock()
	defer u.m.Unlock()

	if u.timer == nil {
		return errors.New("uploader is closed")
	}

	currentSize := len(prefix) + u.bs.Len() + len(suffix)
	newBytesSize := len(bs) + len(delimiter)

	if currentSize+newBytesSize > int(u.opts.maxBatchSize) {
		if err := u.flush(true); err != nil {
			return err
		}
	}

	if u.bs.Len() != 0 {
		bs = append(delimiter, bs...)
	}

	_, err := u.bs.Write(bs)
	if err != nil {
		u.logger.WithError(err).Errorf("Dropped message (%d bytes)", len(bs))
	} else {
		u.logger.Tracef("Added message (%d bytes) to buffer", len(bs))
	}
	return err
}

func (u *Uploader) flush(alreadyAcquiredLock bool) error {
	if !alreadyAcquiredLock {
		u.m.Lock()
		defer u.m.Unlock()
	}

	defer u.timer.Reset(u.opts.flushPeriod)

	if u.bs.Len() == 0 {
		return nil
	}

	toUpload := append(prefix, u.bs.Bytes()[:]...)
	toUpload = append(toUpload, suffix...)
	u.bs.Reset()

	u.activeRequests.Add(1)

	// Write in a go routine to be non blocking (unless the app is stopping)
	go func() {
		defer u.activeRequests.Done()

		if err := u.client.send(toUpload); err != nil {
			u.logger.WithError(err).Errorf("Failed to send batch (%d bytes)", len(toUpload))
		} else {
			u.logger.Tracef("Uploaded batch (%d bytes)", len(toUpload))
		}
	}()

	return nil
}
