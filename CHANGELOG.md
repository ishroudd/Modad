# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
- Major redesign to the way mod files work, hence the decision not to release as v1 yet.

## [0.0.9] - 2020-08-11
### Added
- Can now read multiple .mod files in a ./mod directory.
- Mods can now be multiple lines. Each line is in "ADDRESS BYTES" format. ADDRESS is a DWORD-sized RVA from the run-time image base, BYTES are the big-endian hexcode bytes to write to it. See the example.mod file.
- Lines can now specify a variable number of bytes to write, up to 7. Note that alphabetical read order is important, b.mod can overwrite any bytes written by a.mod.

## [0.0.1] - 2020-08-05
### Added
- Dragons
