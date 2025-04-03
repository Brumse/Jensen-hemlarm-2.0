# Raspberry Pi Pico W WiFi Setup (C++)

Detta projekt visar hur du ställer in Raspberry Pi Pico W för att ansluta till ett WiFi-nätverk med C++.

## Beroenden

* Raspberry Pi Pico SDK
* CMake
* Make

## Installation

1.  **Klona repot:**

    ```bash
    git clone [repo-länk]
    ```

2.  **Skapa headerfil med WiFi-uppgifter:**
    ```bash
    cat > include/wifi_credentials.h << EOF
    #pragma once
    #include <string>

    const std::string WIFI_SSID = "change to your SSID";
    const std::string WIFI_PASSWORD = "change to your WiFi password";
    EOF
    ```

    **Viktigt:** Byt ut `"change to your SSID"` och `"change to your WiFi password"` med dina faktiska WiFi-uppgifter. 
    
    **Var försiktig med att inte lägga upp denna fil på ett publikt git repo!**.

3.  **Konfigurera CMake:**
    ```bash
    cmake -B build
    ```

4.  **Bygg projektet med Make:**

    ```bash
    make -C build
    ```

    Den kompilerade `.uf2`-filen skapas i `build/bin`.

5.  **Bootload Pico W:**

    Sätt din Pico W i bootload-läge genom att hålla ner BOOTSEL-knappen och ansluta den till din dator. Kopiera eller flytta `.uf2`-filen till Pico W.


6.  **Hitta Pico W på nätverket:**

    Använd `arp-scan` eller `nmap` för att hitta Pico W på ditt nätverk.

    ```bash
    sudo arp-scan --localnet
    ```

    Eller logga in på din router för att se anslutna enheter.


## Felhantering

* Om anslutningen misslyckas, kontrollera att ditt SSID och lösenord är korrekta.
* Om Pico W inte hittas på nätverket, kontrollera att den är korrekt bootloadad och att ditt WiFi-nätverk fungerar.


