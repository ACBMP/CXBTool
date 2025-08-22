# CXB Tool

This is a simple CLI program to edit the CXB file for an ACB RDV server. Thanks to Kamzik123 for the help reversing the CXB format.

## Usage

To export all the XML files from a gamesettings CXB file to a folder named xml_folder:

```
CXBTool export gamesettings_c1380_d873_s6285.cxb xml_folder
```

To convert back run:

```
CXBTool convert xml_folder gamesettings_c1380_d873_s6285.cxb
```

You can then place the CXB file in the folder with your ACB RDV server and your updated CXB will be sent to clients upon login.

## Compilation

```
meson setup build
ninja -C build
```

For cross-compiling to Windows:

```
meson setup build-windows --cross-file=mingw-w64-x86_64.txt --default-library=static --buildtype=release --reconfigure
ninja -C build-windows
```

