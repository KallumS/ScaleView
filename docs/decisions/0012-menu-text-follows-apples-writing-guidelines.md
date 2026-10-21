# 0012. Write the menus to Apple's writing guidelines

Status   - Accepted
Date     - 2026-10-14
Commits  - the `claude/adoring-feynman-bihxp2` branch (version 1.0.4)

## Context

The user asked for every repository's on-screen text to follow Apple's Human
Interface Guidelines on writing
(developer.apple.com/design/human-interface-guidelines/writing), as was done
for Noterator and Miderator in their decision 0053. ScaleView's text is two
menus and the label under the circles, and it nearly did already.

## Decision

The same rules as Noterator's 0053:

- **Menu items and submenu names** in title-style capitalization, with a
  verb first where the item acts. "Random Scale" did something without
  saying so, and is now **Choose a Random Scale**. Toggles with a tick keep
  their names (Show Note Names, Simplify Note Names, Dock Window).
- **An ellipsis (…)** only on an item that asks for more before it acts.
  Nothing here does.
- **The label** under the circles, when it is not a key or a chord, is a
  sentence-case fragment with no full stop: "No scale selected", "MIDI input
  unavailable".
- **Pro is the reference.** Simple carries Pro's menu words exactly, and the
  plugin (`KallumS/ScaleView`) carries them too, where it has the same item.

Left alone on purpose: the scale and root names and the colour names (they
are tables copied across repositories, and stored by name - see 0007), the
chord symbols (notation, not prose), the ExtState keys and the script file
names.

## Consequences

- A menu item added later is written the same way, in Pro first, then copied
  to Simple and the plugin.
- A literal `&` in a `gfx.showmenu` item is written `&&`.

## Evidence

Both suites open the options menu by finding "Choose a Random Scale" in it,
so a menu that loses the item fails them; run against the old scripts they
report two failures each.
