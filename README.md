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


6. Kör igång minicom för att lyssna på din device
```bash
    minicom -D /dev/ttyACM0 -b 115200
```
    kolla upp vad din device heter och byt ut mot ttyACM0 (som min heter)
    prova att koppla in picon och kör nedan kommando och dra ur den och kör den igen för att lokalisera din device.
```bash
    ls /dev
```
7. SSH:a in i din rasp zero och installera mosquitto och mosquitto client
```bash 
    sudo apt install mosquitto mosquitto-clients -y
``` 
8. Lägg in information i mosquitto conf för att kunna lyssna och välj rätt port.
```bash
    sudo vim /etc/mosquitto/mosquitto.conf
```
    lägg in detta längst ner i filen:
```bash 
    allow_anonymous true
    listener 1883 0.0.0.0
```
9. kör en subscribe på ämnet
```bash 
    mosquitto_sub -t /alarm/offline -v
```
10. kör en till sub via ett annat terminalfönster 
```bash
    mosquitto_sub -t /messagepub -q 2
```
## Felhantering

* Om anslutningen misslyckas, kontrollera att ditt SSID och lösenord är korrekta.
* Om Pico W inte hittas på nätverket, kontrollera att den är korrekt bootloadad och att ditt WiFi-nätverk fungerar.
