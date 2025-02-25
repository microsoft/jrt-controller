// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

package jrtcbindings

import "github.com/google/uuid"

var (
	// StreamIDNil empty UUID, all zeros
	StreamIDNil = StreamID(uuid.Nil)
)

// StreamIDNew creates a new random UUID or panics.
func StreamIDNew() StreamID {
	return StreamID(uuid.New())
}

// String returns the string form of uuid, xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
// , or "" if uuid is invalid.
func (s StreamID) String() string {
	return uuid.UUID(s).String()
}

// MarshalBinary implements encoding.BinaryMarshaler.
func (s StreamID) MarshalBinary() ([]byte, error) {
	return uuid.UUID(s).MarshalBinary()
}

// UnmarshalBinary implements encoding.BinaryUnmarshaler.
func (s *StreamID) UnmarshalBinary(data []byte) error {
	return (*uuid.UUID)(s).UnmarshalBinary(data)
}

// StreamIDFromBytes creates a new UUID from a byte slice. Returns an error if the slice
// does not have a length of 16. The bytes are copied from the slice.
func StreamIDFromBytes(b []byte) (s StreamID, err error) {
	err = s.UnmarshalBinary(b)
	return s, err
}

// StreamIDParse decodes s into a UUID or returns an error if it cannot be parsed.  Both
// the standard UUID forms defined in RFC 4122
// (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx and
// urn:uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx) are decoded.  In addition,
// Parse accepts non-standard strings such as the raw hex encoding
// xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx and 38 byte "Microsoft style" encodings,
// e.g.  {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}.  Only the middle 36 bytes are
// examined in the latter case.  Parse should not be used to validate strings as
// it parses non-standard encodings as indicated above.
func StreamIDParse(s string) (StreamID, error) {
	out, err := uuid.Parse(s)
	return StreamID(out), err
}

// StreamIDMustParse is like Parse but panics if the string cannot be parsed.
// It simplifies safe initialization of global variables holding compiled UUIDs.
func StreamIDMustParse(s string) StreamID {
	return StreamID(uuid.MustParse(s))
}
