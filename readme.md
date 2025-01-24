# GDownload

GDownload is a multi-platform download manager built with C++ and Qt, leveraging the power of aria2c for its core downloading functionality.

## Features

- User-friendly interface built with Qt
- Cross-platform support (Windows, macOS, Linux)
- Powered by aria2c for efficient and fast downloads
- Multi-threaded downloading
- Support for various protocols (HTTP, HTTPS, FTP, BitTorrent, Metalink)
- Resume interrupted downloads

## Requirements

- C++ compiler
- Qt framework
- aria2c

## Building

(Add specific build instructions here)

## Usage

1. Launch the GDownload application
2. Enter the URL of the file you want to download
3. Choose the destination folder
4. Click "Start Download"

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

GDownload 使用 [Apache License 2.0](LICENSE.txt) 开源协议。

### 第三方组件

本项目使用了多个优秀的开源组件，包括：

- Qt Framework (LGPL v3)
- FramelessHelper (MIT)
- Boost Libraries (Boost Software License)
- LibtorrentRasterbar (BSD)
- PugiXML (MIT)

详细的第三方组件信息和许可证声明请查看 [NOTICE](NOTICE) 文件。

### 许可证合规性

- 本项目完全遵守所有第三方组件的许可证要求
- 对于 Qt LGPL 协议，我们采用动态链接方式以确保合规
- 所有第三方组件的原始许可证文本都已包含在发布包中

如果您计划使用、修改或分发本项目，请确保同时遵守：
1. 本项目的 Apache License 2.0 协议
2. 所有第三方组件的相应许可证要求

## Acknowledgements

- [Qt](https://www.qt.io/)
- [aria2](https://aria2.github.io/)
