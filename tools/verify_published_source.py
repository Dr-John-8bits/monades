#!/usr/bin/env python3
"""Compare the browsable 1.1 source with the original published archive."""
from hashlib import sha256
from pathlib import Path, PurePosixPath
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
ARCHIVE = ROOT / "Mutable-Dreams-1.1-Source.zip"
EXPECTED = "90fef68214bd1ff6674db2c80852cc2291b31321e9c26cb20cf59d45ee04af40"
README = "docs/release-1.1/README-from-source-archive.md"


def main():
    if sha256(ARCHIVE.read_bytes()).hexdigest() != EXPECTED:
        raise SystemExit("FAIL: the published source archive has changed")
    mismatches = []
    count = 0
    with ZipFile(ARCHIVE) as archive:
        for entry in archive.infolist():
            if entry.is_dir():
                continue
            parts = PurePosixPath(entry.filename).parts
            if parts[0] != "mutable-dreams-1.1" or ".." in parts:
                raise SystemExit(f"FAIL: unexpected archive path {entry.filename}")
            relative = PurePosixPath(*parts[1:]).as_posix()
            target = ROOT / (README if relative == "README.md" else relative)
            if not target.is_file() or target.read_bytes() != archive.read(entry):
                mismatches.append(relative)
            count += 1
    if mismatches:
        raise SystemExit("FAIL: source differences:\n" + "\n".join(mismatches))
    print(f"PASS: all {count} archived files match byte for byte")
    print(f"Original archive README is preserved at {README}")
    print("Additional repository documentation is outside the archive comparison.")


if __name__ == "__main__":
    main()
