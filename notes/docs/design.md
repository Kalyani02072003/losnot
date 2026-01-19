
# losnot — Design Decisions

---

## Why C + GTK?

- Native to Linux desktops
- No runtime dependencies
- Fine-grained control over windows
- Educational value (real systems programming)

losnot is intentionally not Electron, Qt, or web-based.

---

## Why Plain Text Storage?

- Users can inspect/edit notes without the app
- No vendor lock-in
- Safer for sensitive data (no hidden formats)
- Easier recovery if app crashes

---

## Why Autosave on Every Keystroke?

- Notes are small
- Disk IO is negligible
- Eliminates data loss edge cases
- Simplifies mental model for users

---

## Why No Cloud Sync?

This is a **local-first tool**.

Sync introduces:
- trust issues
- credential management
- security risks
- unnecessary complexity

Users who want sync can manage it themselves (git, rsync, etc.).

---

## Why Minimal UI?

losnot is meant to:
- stay out of the way
- reduce cognitive load
- feel like a utility, not an app

Visual polish is intentionally subtle.

---

## Known Tradeoffs

- Drag behavior differs across X11/Wayland
- Window stacking not yet managed
- No encryption (planned, optional)

---

## Project Direction

losnot prioritizes:
1. Correctness
2. Transparency
3. Learnability
4. Stability

Features will only be added if they do not compromise these.
