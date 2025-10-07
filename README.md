# Bakalářská práce – Návrh a implementace softwarového rozhraní pro chytré hodinky s využitím procesoru ESP32

Tento repozitář obsahuje zdrojové kódy k bakalářské práci s názvem  
**"Návrh a implementace softwarového rozhraní pro chytré hodinky s využitím procesoru ESP32"**

## Stručný popis projektu

Tento archiv obsahuje kompletní zdrojový kód, konfigurační soubory a návod ke kompilaci pomocí vývojového prostředí PlatformIO ve Visual Studio Code.

## Požadavky pro kompilaci

Před kompilací je nutné mít nainstalováno následující:

- **GCC/G++** – překladač jazyka C/C++
- **Visual Studio Code**
- **Rozšíření PlatformIO** pro VSCode  
  (lze nainstalovat z integrovaného obchodu s rozšířeními ve VSCode)

## Kompilace projektu

1. **Otevřete Visual Studio Code.**

2. **Otevřete projektovou složku**, která obsahuje soubor `platformio.ini`.

3. PlatformIO automaticky inicializuje projekt (stažení závislostí, konfigurace prostředí).  
   To může chvíli trvat při prvním otevření.

4. Jakmile je projekt připraven, klikněte v levém panelu na ikonu PlatformIO a vyberte **"Build"**,  
   nebo stiskněte klávesovou zkratku: Ctrl + Alt + B (Windows/Linux) Cmd + Alt + B (macOS)

5. Výstup kompilace se zobrazí ve spodní části okna. Pokud proběhne úspěšně, projekt je připraven ke spuštění.

## Spuštění projektu

Po úspěšné kompilaci je možné projekt nahrát do cílového zařízení (např. **ESP32**, **T-Watch S3**)  
pomocí tlačítka **"Upload"** v PlatformIO. Zařízení musí být připojeno k počítači přes USB.

## Autor

**Martin Burian**  
Bakalářská práce – 2025  
VŠB – Technická univerzita Ostrava  
Fakulta elektrotechniky a informatiky
