# 2026-10-21 - Apple's HIG Foundations, the final sweep (1.2.0)

The user asked for a last pass over all 19 Foundations pages in every repo.
For this icon most of the checklist is met or does not apply; the table is
in `docs/apple-hig-review.md`, *Final sweep*.

## What changed

- **A rim on every unlit circle** (`COLOR_EDGE`, `#6D7581`, 3.2:1). The dark
  circles were 1.4:1 on the background. The rim is a filled circle under the
  fill rather than an outline, because the suites treat every unfilled
  stroke as the played-note ring and assert it is `#FFF200` - an outline
  would have meant teaching six ring checks to tell the two apart.
- **The system face** (`pickFace()`), Orchestration Helper's way: ask, then
  believe only `gfx.getfont()`. The token `"system"` was left out of what
  counts as San Francisco: Windows can answer "System" for a face it does
  not know.
- Pro first, then Simple, the same lines. Version 1.2.0 for both.

## Checks

Both mocks answer `gfx.getfont` the way an OS does (a face it lacks becomes
the first it has). The new checks: the face with three sets of system faces,
and every circle's outer edge 3:1 with every highlight. Simple's
every-root-and-scale sweep counted circles in drawing order and had to learn
that an unlit circle is now drawn twice. Ten sabotages, five per script, all
caught, in a scratch copy.

## Not done

- Not run in REAPER: the name REAPER reports for San Francisco, and how
  "Bbb" fits at 10 points in it.
- The plugin (`KallumS/ScaleView`) could take the rim too; not touched.
