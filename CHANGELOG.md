# Changelog

## v1.0.5 (2025-11-09)

### ✨ New Features

**Download Management**
- Smart delete confirmation - choose to delete files or keep them when removing tasks
- Post-download automation - play sounds, open files, run commands, or shutdown system on task completion
- Browser extension helper - easy configuration for Chrome/Edge/Firefox with one-click RPC setup

**Advanced Settings**
- Timeout & retry configuration
- BitTorrent advanced options (DHT, peer limits, encryption)
- User-Agent presets and customization
- Speed control and connection limits
- Custom Aria2 RPC port and secret key

**UI Components**
- Close confirmation dialog with "minimize to tray" option
- Element Plus style message system
- Enhanced message box component

### 🎨 UI/UX Improvements

**Design System**
- Migrated to Element Plus design standards across all pages
- Modern blue color scheme with improved dark mode contrast
- Refined spacing and layout consistency
- Optimized download item hover states and visual hierarchy

### 🔧 Core Enhancements

- GitHub proxy domain rotation for better download stability
- Tracker list sync with ETag caching and CDN fallback
- Config format migration from INI to TOML (with auto-migration)
- Multi-platform CI/CD workflow consolidation

### 🌍 Localization

- Improved Japanese translation
- Updated UI icons

### 🐛 Bug Fixes

- Fixed Windows path escaping in process relaunch
- Corrected card padding calculations
- Various UI layout and spacing adjustments

---

**Full Changelog**: https://github.com/cool2528/GDownload/commits/v1.0.5
