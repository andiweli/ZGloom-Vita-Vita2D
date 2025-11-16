# ZGloom-Vita-Vita2D <br/> GPU-accelerated Amiga **Gloom** Port for PS Vita / PSTV

> A Vita2D-based, GPU-accelerated port of the classic Amiga FPS **Gloom** (including Gloom Deluxe, Gloom 3 and Zombie Massacre) for PlayStation Vita and PSTV. Based on version 3.0 of the original SDL ZGloom-Vita port.

[![Latest release](https://img.shields.io/github/v/release/andiweli/ZGloom-Vita-Vita2D?label=latest%20Vita%20release)](https://github.com/andiweli/ZGloom-Vita-Vita2D/releases/latest)
[![Platform](https://img.shields.io/badge/platform-PS%20Vita%20%2F%20PSTV-blue)](https://github.com/andiweli/ZGloom-Vita-Vita2D)
[![Engine](https://img.shields.io/badge/renderer-Vita2D%20GPU-brightgreen)](https://github.com/andiweli/ZGloom-Vita-Vita2D)

**ZGloom-Vita-Vita2D** is a Vita2D version of [ZGloom-Vita-SDL](https://github.com/andiweli/ZGloom-Vita-SDL) with GPU rendering.  
It brings the Amiga cult FPS **Gloom** and its successors to PS Vita / PSTV with a fixed renderer, stable framerate options and extra visual effects – while staying faithful to the original gameplay.

---

## ✨ Key Features (Vita2D & v3.0 Improvements)

Compared to the original Vita SDL port, this Vita2D version adds and improves:

- 🧮 **Optimized & fixed renderer (GPU-based)**  
  – Vita2D GPU rendering for smoother visuals  
  – Optimized and cleaned-up renderer code

- ⏱️ **Stable framerate options (50 / 30 FPS)**  
  – In-game option to choose between **50 FPS** and **30 FPS**  
  – Both modes tuned for stable framerates on PS Vita / PSTV

- 🩸 **Improved blood & visibility logic**  
  – Blood is now correctly **occluded by walls** when enemies are beside or behind geometry

- 🔫 **Correct muzzle flash behavior**  
  – Fixed bug where sideways movement caused the muzzle flash to appear from the side instead of the weapon

- 🎛️ **Better menu navigation & controls**  
  – DPAD navigation now **loops** through menu entries  
  – **Circle** = go back  
  – **Square** = value lower  
  – **Cross** = value higher

- 🎨 **Atmospheric post-processing overlays**  
  – Optional **vignette** overlay  
  – Optional **film grain**  
  – Optional **scanlines**  
  – All fully configurable through the in-game options menu

- ⚙️ **Toolchain & size optimizations**  
  – Toolchain finetuning for up to **~4% smaller `.vpk` size**  
  – Slight performance improvements thanks to cleaned-up dependencies

- 🔊 **Higher-quality audio defaults**  
  – `SDL_mixer` and LibXMP player configured for **44 kHz** (previously 22 kHz) for improved sound quality

- 🧹 **General cleanup & UX polish**  
  – Menu items and descriptions revised and optimized  
  – All compilation warnings removed  
  – Non-used dependencies cleaned up

---

## 🕹 What is Gloom?

[Gloom](https://en.wikipedia.org/wiki/Gloom_(video_game)) was a 1995 Doom-like first-person shooter from **Black Magic Software** for the Commodore Amiga. It featured very messy and meaty graphics and required a powerful Amiga at the time (an A1200 with 030 CPU was still on the low end).  

The engine later powered several related games and successors, including:

- **Gloom Deluxe / Ultimate Gloom** – enhanced graphics and effects  
- **Gloom 3**  
- **Zombie Massacre**  
- Various full-game conversions of other 90’s Amiga titles

ZGloom is a modern reimplementation of this engine, and this project is the **PS Vita / PSTV Vita2D port** of that reimplementation.

---

## 🖼 Screenshot

Some screenshots of ZGloom-Vita-Vita2D running on PS Vita:

![ZGloom-Vita-Vita2D – PS Vita gameplay mockup](https://github.com/user-attachments/assets/98efe43e-9a7a-4fc7-87bf-7463df071cb5)

---

## 📥 Download

You can download the latest pre-built **`.vpk`** here:

👉 **[Latest ZGloom-Vita-Vita2D release](https://github.com/andiweli/ZGloom-Vita-Vita2D/releases/latest)**

The release supports:

- **Gloom (Classic)**
- **Gloom Deluxe / Ultimate Gloom**
- **Gloom 3**
- **Zombie Massacre**

You still need the original game data files (see below).

---

## 📖 How to Play Gloom on PS Vita / PSTV

### 1. Get the original Gloom game data

Gloom was made freely available by its developers [here](#).  
Other official games using the Black Magic **Gloom** engine such as [Gloom 3](https://aminet.net/package/game/demo/gloom3_preview) and [Zombie Massacre](https://aminet.net/package/game/shoot/ZombieMassacre) are available on **Aminet**.  

You can:

- Use files from your **original game installation**, or  
- Download the free releases (thanks to **Gareth Murfin** for permission)

### 2. Install the `.vpk` on your Vita / PSTV

1. Download the latest `.vpk` from the **Releases** page.  
2. Install the `.vpk` on your PS Vita / PSTV using your preferred homebrew method (VitaShell, etc.).

### 3. Copy the game data to the correct folders

After extracting the game data on your PC, copy the *folders* (depending on which games you want to play) to the following locations on your Vita:

> ux0:/data/zgloom/gloom  
> ux0:/data/zgloom/deluxe  
> ux0:/data/zgloom/gloom3  
> ux0:/data/zgloom/massacre  

Once the files are in place, launch **ZGloom-Vita-Vita2D** from the LiveArea and select the game you want to play.

---

## 🔊 In-Game Music (Module Support via XMP)

ZGloom can play in-game music using any module format supported by **XMP** (e.g. `.mod`, `.xm` etc.).

1. Put your module files into the **`sfxs`** folder of the game data.  
2. Add a line like the following to the game script:

    song_blitz.mod

You can use multiple `song_` commands in the script, which enables **per-level music**, each level using its own track.

---

## 🛠 Building From Source (Linux / WSL + VitaSDK)

If you want to build the `.vpk` yourself, you can use **Linux** or **WSL** (Windows Subsystem for Linux) plus VitaSDK.

I am using **Ubuntu on Windows** with **VitaSDK**.

### 1. Install required packages

    apt-get install make git-core cmake python

### 2. Install VitaSDK

Follow the instructions on [VitaSDK.org](https://vitasdk.org) (see "VitaSDK Installation" and "porting libraries" sections):

    export VITASDK=/usr/local/vitasdk
    export PATH=$VITASDK/bin:$PATH    # add vitasdk tools to $PATH
    git clone https://github.com/vitasdk/vdpm
    cd vdpm
    ./bootstrap-vitasdk.sh
    ./install-all.sh

### 3. Install the LibXMP Vita library

Install the **LibXMP** Vita port (see the [VitaSDK library porting section](https://vitasdk.org) for detailed instructions).

### 4. Build the .vpk

Inside the repository root:

    ./build.sh

This will generate the `.vpk` package that you can install on your PS Vita / PSTV.

---

## 📜 License & Third-Party Code

The licensing situation around the original **Gloom** source is a bit unusual:

> The Gloom source release says only the `.s` and `.bb2` files are open source, but the Gloom executable bakes in some maths lookup tables (generated by the `.bb2` files), bullet and sparks graphics, and the title screen for Classic Gloom.

This port follows the original release and **does not** include the Classic Gloom title screen and similar executable-bundled assets. Instead, it may display alternative imagery (e.g. the **Black Magic** logo image).

### Libraries and Tools Used

- **LibXMP** – MED / module playback  
  – http://xmp.sourceforge.net/  

- **SDL2** and **SDL2_mixer** – cross-platform media & audio (for sound)  
  – https://www.libsdl.org/  

- **DeCrunchmania** C code by Robert Leffman (license unknown)  
  – http://aminet.net/package/util/pack/decrunchmania_os4  

- **VitaSDK** – open SDK for PS Vita homebrew  
  – https://vitasdk.org/

---

## ℹ️ About This Project

**ZGloom-Vita-Vita2D** is:

- A Vita2D version of ZGloom for **PS Vita** and **PSTV**  
- Based on the SDL Vita port (version 3.0)  
- Compatible with **Gloom (Classic)**, **Gloom Deluxe / Ultimate Gloom**, **Gloom 3** and **Zombie Massacre**

It aims to provide a **fast, GPU-accelerated, and polished Vita experience** of the classic Amiga Gloom engine, with modern conveniences and visual tweaks but without changing the core gameplay.

**Keywords / Topics:**  
_amiga • gloom • vita • psvita • homebrew • zgloom • gloomdeluxe • zombiemassacre • vita2d • ps tv shooter_

If you enjoy it, feel free to ⭐ star the repo so other PS Vita & Amiga fans can find it more easily.
