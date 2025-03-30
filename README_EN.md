# GDownload

<p align="center">
  <img src="src/App/ui/Resource/images/logo/logo.svg" alt="GDownload Logo" width="128" height="128"/>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-Apache%202.0-blue.svg" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/platform-Windows%20|%20macOS%20|%20Linux-lightgrey.svg" alt="Platform"></a>
  <a href="#"><img src="https://img.shields.io/badge/language-C++20-orange.svg" alt="Language"></a>
  <a href="#"><img src="https://img.shields.io/badge/Qt-6.5+-green.svg" alt="Qt Version"></a>
</p>

<p align="center">
  <a href="https://github.com/cool2528/GDownload/stargazers">
    <img src="https://img.shields.io/github/stars/cool2528/GDownload?style=social" alt="Stars">
  </a>
  <a href="https://github.com/cool2528/GDownload/network/members">
    <img src="https://img.shields.io/github/forks/cool2528/GDownload?style=social" alt="Forks">
  </a>
</p>

English | [简体中文](README.md)

GDownload is a modern cross-platform download manager built with C++ and Qt. It combines modern technology stack and excellent open-source components to provide users with an efficient and stable downloading experience.

## ✨ Features

- 🖥️ Cross-platform support (Windows, macOS, Linux)
- ⚡ Efficient download engine powered by aria2c
- 🚀 Multi-threaded concurrent downloads
- 🔄 Support for multiple protocols (HTTP, HTTPS, FTP, BitTorrent, Metalink)
- 📱 Download resume capability
- 🎨 User-friendly graphical interface
- 🗂 Support for Baidu Netdisk shared link parsing and downloading (only standard speed download is supported, for high-speed downloads please subscribe to Baidu Netdisk official SVIP)

## 🛠️ Technology Stack

- 🎯 UI Framework: Qt Quick (QML) + Qt C++
- ⚙️ Core Engine: aria2c
- 🌐 Network Library: Boost.Asio
- 🔗 BT Download: LibtorrentRasterbar
- 📄 XML Parser: PugiXML
- 🪟 Frameless Window: FramelessHelper

## 📦 Installation

[Releases](https://github.com/cool2528/GDownload/releases) Page to download the latest version

### macOS Troubleshooting

If you encounter "App is damaged" or "Cannot open application" warnings on macOS, this is due to security restrictions for applications without developer signatures. You can resolve this by:

1. Open "System Preferences" > "Security & Privacy" > "General", and click "Open Anyway" button (if shown)
2. If the above method doesn't work, open Terminal and enter the following command:
   ```
   sudo xattr -r -d com.apple.quarantine /Applications/GDownload.app
   ```
   Note: Please replace the path with your actual installation location

3. Enter your administrator password, then try opening the application again

## 🚀 Quick Start

1. Launch GDownload
2. Enter the URL of the file you want to download
3. Choose the destination folder
4. Click "Start Download"

## 🤝 Contributing

Contributions are welcome! Feel free to submit a Pull Request or create an Issue.

## 📄 License

GDownload is licensed under the [Apache License 2.0](LICENSE.txt).

### Third-Party Components

This project uses several excellent open-source components, including:

- Qt Framework (LGPL v3)
- FramelessHelper (MIT)
- Boost Libraries (Boost Software License)
- LibtorrentRasterbar (BSD)
- PugiXML (MIT)

For detailed third-party component information and license notices, please check the [NOTICE](NOTICE) file.

## 🌟 Acknowledgements

Thanks to all developers and users who have contributed to this project!

## 📱 Contact Us

- [GitHub](https://github.com/cool2528/GDownload)
- [Issue Tracking](https://github.com/cool2528/GDownload/issues)

## ⚠️ Disclaimer

GDownload is provided solely as a download tool for users to legally download Internet resources. Please comply with local laws and regulations when using this software.

- This software does not collect any user privacy information
- The copyright of all resources downloaded by users belongs to the original author or their legal holders
- The developers are not responsible for the content downloaded by users, nor for any losses or damages that may result from using this software
- The Baidu Netdisk shared link parsing function is only intended for legally accessing your own files and should not be used to infringe on others' intellectual property rights
- If any function violates relevant laws and regulations, please contact us promptly through Issues, and we will address it immediately

By using this software, you acknowledge that you have read and agreed to all terms of this disclaimer.