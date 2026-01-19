
# losnot — local sticky notes for Linux

**losnot (LOcal Sticky NOtes Tool)** is a minimal, privacy-first sticky notes widget for Linux desktops (GNOME/GTK).

It provides lightweight, always-on-screen notes backed by **local files only** no cloud, no sync, no accounts.  
Ideal for quick references like tokens, commands, temporary passwords, or scratch notes you don’t want online.

> losnot is intentionally simple: one widget = one local file.

---

## Features

- Sticky note widgets (borderless GTK windows)
- Autosave on every edit
- Notes stored as plain text files (filesystem = database)
- Create multiple notes
- Rename notes (renames file + UI title)
- Open notes directory from UI
- Cannot accidentally close the last note
- Minimal dark, terminal-like theme
- Session disable / permanent disable (autostart)
- Designed for extensibility and learning GTK internals

---

## Current Limitations (Known Issues)

These are intentional tradeoffs and work-in-progress areas:

- Notes currently stack on top of each other (z-order management pending)
- Dragging is temporarily unstable on some systems (GTK / Wayland differences)
- No encryption at rest (planned, optional)
- No restore of multiple notes on startup yet

All of these are documented and tracked.

---

## Storage Layout

```text
~/.config/losnot/
├── config.ini        # window position, autostart flag
└── autostart/
    └── losnot.desktop

./notes/
├── Untitled-2026-01-19-184212.txt
├── api-token.txt
└── scratch.txt
```

---

## Build and Run

### Dependencies

* gcc
* GTK 3
* glib

```bash
sudo apt install build-essential libgtk-3-dev
```

### Build

```bash
make clean
make
```

### Run (temporary workaround for snap / glibc)

```bash
env -i \
HOME=$HOME \
PATH=/usr/bin:/bin \
DISPLAY=$DISPLAY \
XAUTHORITY=$XAUTHORITY \
DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS \
./losnot
```

---

## Philosophy

losnot is:

* Local-first
* Auditable
* Hackable
* Minimal by design

This project also serves as a learning reference for:

* GTK window management
* Linux desktop integration
* Filesystem-based state
* Event-driven UI in C

---

## High-Level Overview

losnot is a **single-process GTK application**.

Each sticky note:

* Is a GTK window
* Owns exactly one local text file
* Autosaves content via `GtkTextBuffer` signals

There is no central note manager or database.

---

## Module Structure

```text
src/
├── main.c       # GTK init and main loop
├── app.c        # application bootstrap
├── window.c     # window creation entry point
├── note.c       # core note logic (UI and storage)
```

---

## Configuration

Configuration is handled via **GLib GKeyFile**.

```ini
[window]
x=1200
y=80
width=320
height=220

[app]
autostart=true
```

Stored at:

```text
~/.config/losnot/config.ini
```

---

## Startup Flow

1. `main.c` initializes GTK
2. `app.c` creates the first note window
3. Configuration is loaded
4. Window position is restored
5. Autosave begins immediately

