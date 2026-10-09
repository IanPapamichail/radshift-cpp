import struct
from pathlib import Path

counts = (0, 1, 2, 6)
bin_duration_ns = 1_000_000_000

header = struct.pack(
    "<8sIIQQ",
    b"RADCOUNT",
    1,
    32,
    len(counts),
    bin_duration_ns,
)

payload = struct.pack("<4I", *counts)

project_root = Path(__file__).resolve().parent.parent
output = project_root / "examples" / "four_bins.rct"

output.parent.mkdir(parents=True, exist_ok=True)
output.write_bytes(header + payload)

print(f"Created: {output}")
print(f"Bytes: {output.stat().st_size}")
