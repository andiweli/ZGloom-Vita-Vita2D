# ZGloom-Vita-Vita2D – GPU-accelerated Amiga Gloom port for PS Vita / PSTV

Vita2D-based, GPU-accelerated port of the modern **ZGloom** engine, bringing the classic Amiga FPS **Gloom** and its successors to PlayStation Vita and PSTV.

> Play Gloom, Gloom Deluxe, Gloom 3 and Zombie Massacre on PS Vita / PSTV with a fixed renderer, widescreen support, post-processing overlays and save/load position – while staying faithful to the original Amiga gameplay.

[![Latest release](https://img.shields.io/github/v/release/andiweli/ZGloom-Vita-Vita2D?label=latest%20Vita%20release)](https://github.com/andiweli/ZGloom-Vita-Vita2D/releases/latest)
[![Platform](https://img.shields.io/badge/platform-PS%20Vita%20%2F%20PSTV-blue)](https://github.com/andiweli/ZGloom-Vita-Vita2D)
[![Engine](https://img.shields.io/badge/renderer-Vita2D%20GPU%20%2B%20LibXMP-brightgreen.svg)](https://github.com/andiweli/ZGloom-Vita-Vita2D)

ZGloom-Vita-Vita2D is the Vita2D follow-up to the original SDL ZGloom-Vita port, focusing on stable framerates, improved rendering and a console-style experience on Sony’s handhelds.

For other platforms, see the companion projects [ZGloom-x86 (Windows)](https://github.com/Andiweli/ZGloom-x86), [ZGloom-Android](https://github.com/Andiweli/ZGloom-Android) and [ZGloom-macOS](https://github.com/Andiweli/ZGloom-macOS).

---

## 🕹 What is Gloom?

[Gloom](https://en.wikipedia.org/wiki/Gloom_(video_game)) was a 1995 Doom-like first-person shooter from **Black Magic Software** for the Commodore Amiga. It featured very messy and meaty graphics and required a powerful Amiga at the time (an A1200 with 030 CPU was still on the low end). The engine later powered several related games and successors, including:

- **Gloom Deluxe / Ultimate Gloom** – enhanced graphics and effects  
- **Gloom 3**  
- **Zombie Massacre**  
- Various full-game conversions of other 90’s Amiga titles

ZGloom is a modern reimplementation of this engine.

---

## ✨ Key Features

- Modern source port of the Amiga Gloom engine  
  Runs the original Gloom data files on PlayStation Vita and PSTV using the modern ZGloom C++ engine and a Vita2D GPU-accelerated renderer.

- Supports multiple official games  
  Play **Gloom**, **Gloom Deluxe / Ultimate Gloom**, **Gloom 3** and **Zombie Massacre** (plus selected mods where available).

- Built-in multi-game launcher  
  If more than one game or mod is present, a simple launcher lets you pick what to play at startup.

- 4:3 and 16:9 display modes with FOV control  
  Switch between the classic 4:3 Amiga look and a widescreen 16:9 mode and adjust the field of view to match your handheld or TV.

- Improved renderer, lighting and effects  
  Uses the fixed ZGloom renderer with cleaner perspective, fewer glitches and subtle lighting tweaks, including dynamic muzzle flashes and colored floor reflections under projectiles and weapon upgrade orbs.

- Atmospheric post-processing overlays (optional)  
  Enable vignette, film grain and scanlines for a more gritty, CRT-style presentation without changing gameplay.

- Save/Load position and extended options  
  Save your in-level position (including health, weapon and ammo state) and tweak many more options than in the original Amiga release.

---

## 🖼 Gameplay-Video and Screenshots

https://github.com/user-attachments/assets/30cdd05e-575c-4d97-ba64-6a737678c8e3

<img width="1280" height="1440" alt="Gloom-Screenshots" src="https://github.com/user-attachments/assets/57884915-6e74-482c-b9fe-f7c98e389dc0" />

---

## 📥 Download

You can download the latest pre-built **`.vpk`** here:

👉 **[Latest ZGloom-Vita-Vita2D release](https://github.com/andiweli/ZGloom-Vita-Vita2D/releases/latest)**

The release supports:

- **Gloom (Classic)**
- **Gloom Deluxe / Ultimate Gloom**
- **Gloom 3**
- **Zombie Massacre**

The game including game data files is also available directly on PS VITA via VitaDB app or [here](https://www.rinnegatamante.eu/vitadb/#/).

---

## 📖 How to Play Gloom on PS Vita / PSTV

### 1. Get the original Gloom game data

Gloom was made freely available by its developers [here](#).  
Other official games using the Black Magic **Gloom** engine such as [Gloom 3](https://aminet.net/package/game/demo/gloom3_preview) and [Zombie Massacre](https://aminet.net/package/game/shoot/ZombieMassacre) are available on **Aminet**.  

You can:

- Use files from your **original game installation**, or  
- Download the free releases (thanks to **Alpha Software** for permission)

### 2. Install the `.vpk` on your Vita / PSTV

1. Download the latest `.vpk` from the **Releases** page.  
2. Install the `.vpk` on your PS Vita / PSTV using your preferred homebrew method (VitaShell, etc.).

### 3. Copy the game data to the correct folders

After extracting the game data on your PC, copy the *folders* (depending on which games you want to play) to the following locations on your Vita:

> ux0:/data/ZGloom/gloom  
> ux0:/data/ZGloom/deluxe  
> ux0:/data/ZGloom/gloom3  
> ux0:/data/ZGloom/massacre  

Once the files are in place, launch **ZGloom-Vita** from the LiveArea and select the game you want to play.

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
- background ambience credit goes to Prophet

It aims to provide a **fast, GPU-accelerated, and polished Vita experience** of the classic Amiga Gloom engine, with modern conveniences and visual tweaks but without changing the core gameplay.

**Keywords / Topics:**  
_amiga • gloom • vita • psvita • windows • x86 • android • macos • homebrew • zgloom • gloomdeluxe • zombiemassacre • sdl • libxmp • vita2d • ps tv shooter_

If you enjoy it, feel free to ⭐ star the repo so other PS Vita & Amiga fans can find it more easily.
