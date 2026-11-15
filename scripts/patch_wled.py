from pathlib import Path
import re

fxh = Path("WLED/wled00/FX.h")
text = fxh.read_text(encoding="utf-8")

# Read the actual MODE_COUNT from the checked-out WLED source instead of
# assuming a hard-coded value. The new custom effect is appended as the
# final slot, so it becomes a normal visible effect in the UI.
match = re.search(r"(?m)^#define\s+MODE_COUNT\s+(\d+)\s*$", text)
if not match:
    raise SystemExit("Could not find a unique '#define MODE_COUNT <number>' in WLED/wled00/FX.h")

old_count = int(match.group(1))
if not (1 <= old_count <= 254):
    raise SystemExit(f"Unexpected MODE_COUNT={old_count}; refusing to patch")

new_id = old_count
new_count = old_count + 1
macro_line = f"#define FX_MODE_GARDEN_SMOOTH_RANDOM {new_id}"
new_count_line = f"#define MODE_COUNT {new_count}"

# Refuse a double patch, but print a useful message if it is already there.
if "FX_MODE_GARDEN_SMOOTH_RANDOM" in text:
    m_id = re.search(r"(?m)^#define\s+FX_MODE_GARDEN_SMOOTH_RANDOM\s+(\d+)\s*$", text)
    if m_id:
        print(f"Garden effect already patched at ID {m_id.group(1)}")
        raise SystemExit(0)
    raise SystemExit("Garden effect macro exists in an unexpected form")

start, end = match.span()
replacement = macro_line + "\n" + new_count_line
text = text[:start] + replacement + text[end:]
fxh.write_text(text, encoding="utf-8")

# Store the selected ID/count for the workflow verification step.
Path("WLED/.garden_effect_id").write_text(str(new_id), encoding="utf-8")
Path("WLED/.garden_mode_count").write_text(str(new_count), encoding="utf-8")

print(f"Patched WLED: Garden Smooth Random ID={new_id}, MODE_COUNT {old_count}->{new_count}")
