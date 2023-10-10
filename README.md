# DIY Sticker Vending Machine with NFC (NTAG21* & NTAG424)

[![DIY vending machine demo](https://twitter.com/arcbtc/status/1581186711668678657)](https://twitter.com/arcbtc/status/1581186711668678657)

<blockquote class="twitter-tweet"><p lang="en" dir="ltr">I think this itsy bitsy completely DIY card sticker vending machine (based on <a href="https://twitter.com/hashtag/bitcoinSwitch?src=hash&amp;ref_src=twsrc%5Etfw">#bitcoinSwitch</a>, but with nfc!), by <a href="https://twitter.com/steefbtc?ref_src=twsrc%5Etfw">@steefbtc</a>, is my current favorite thing 🤗 <a href="https://t.co/L7dni3Z1Vf">pic.twitter.com/L7dni3Z1Vf</a></p>&mdash; Ben Arc 🏴󠁧󠁢󠁷󠁬󠁳󠁿✊⚡️ (@arcbtc) <a href="https://twitter.com/arcbtc/status/1581186711668678657?ref_src=twsrc%5Etfw">October 15, 2022</a></blockquote> <script async src="https://platform.twitter.com/widgets.js" charset="utf-8"></script>

## This is a proof of concept of a Vending Machine that accept Bitcoin Lightning payments (with NFC). 

<br>

<p>The goal of this  project to get you and the people around you excited about the possibilities of Bitcoin lightning payments by using it and seeing it work in a real life application.</p>

This little DIY Sticker Vending Machine is inspired by [Ben Arcs](https://github.com/arcbtc) [BitcoinSwitch](https://youtu.be/FeoIwTjv3YM) (github project: [BitcoinSwitch](https://github.com/arcbtc/bitcoinSwitch)), runs with LNBits and accepts Bitcoin Lightning payments in return for stickers. ;)

<p>You can pay for a sticker by scanning the QRCode on the display with your favourite lightning wallet app or tap to the NFC module with a NFC type 2 tag or type 4 (Boltcard) card that contains a LNURL withdrawal link.</p>

<p>The Vending Machine will poll (check every x seconds) if a payment has been received.  When a confirmation of payment is received, the Vending Machine will activate the Sticker dispenser and a sticker will be pushed out of the Vending Machine. Customer happy, Vending Machine happy!<br></p>

### What you need:
- (TTGO) esp32 dev module (Can be any, but for this project I used the TTGO as it has a built in display which is nice for displaying some usage feedback) 
- 360 degrees mini servo motor
- Some (copper) metal wire (1.5mm diameter)
- Karton (to make the boxes of the vending machine)
- USB cables (C or mini usb, depending on your esp32 dev module)
- powerbank
- (Dupont) wires

### Tools:
- Scissors
- Flashlight (or something else that sturdy and tube shaped so you can use it to form the wire in shape)
- (Duck-)tape/glue
- Visual Studio Code IDE
- [PlatformIO](https://platformio.org/platformio-ide)

### Setup:
1. Download the files from this github repository onto your computer.
2. Open the repository folder in Visual Studio Code. 
3. Make sure you have the Platform IO extension installed.
5. Select the esp32 dev module as your board in the board manager.
6. Connect your esp32 dev module with a usb cable to your computer.
7. compile the project
8. upload to the esp32 selecting the right port or use auto
9. Open the serial monitor output window in your VSCode IDE, set the baudrate to 115200 and start monitoring.
10. Press the reset button on your esp32 dev module.
11. You should now be able to see the serial output of the esp32 in the serial monitor window

### Configure your Vending Machine:
To be able to configure the wifi and ports of your esp32 you will have to switch it in Access Point (Portal) mode:
1. If your esp32 fails to connect with the internet on startup it will automatically go into Portal mode and show you the credentials to login on its Wifi network to configure your wifi connection.
2. Get your mobile phone or another device with a browser and wifi. Scan the displayed QR code to login on your esp32 wifi access point. Or use the displayed credentials to fill in manually.
3. A browser with a configuration page should appear now here you can fill in the credentials of the wifi network you want your esp32 to connect to.
4. Configure your ssid for the Vending Machine (set the wifi network and its password that your Vending Machine can use to connect to the internet).
5. Save your credentials. If the esp32 can connect to the wifi network you’re redirected to the succes page. 
8. You can now reset the esp32 again and it will automatically connect to your network. 
9. Then your vending Machine will try to connect to the NFC module. If the nfc module is connected and the esp is in scanning mode, you can now hold a NFC card to the nfc module to scan it.

### Filling the Vending Machine:
1. Reset your esp32 and hold the vendor pin/wire when its starting up.
2. The vending Machine will go into Vendor mode
3. In Vendor mode you can refill and empty your vending machine by using the two buttons on your TTGO
4. While in Vendor Mode you can use these buttons to make the servo spin clock or counter clockwise to fill or empty the machine
5. To exit this mode, reset the machine again and dont touch the vendot pin/wire when doing so ;)

### Using your Vending Machine:
When configured and connected to the internet you can scan a nfc card by holding it close to the nfc module.
Wait for the Vending Machine to tell its done reading before removing the card.
When the card is suceessfully read it will display the scanned url and will check if its a valid lnurlWithdraw. 
If so it will do a withdraw request, if succcesfull and the vending amount is received, the vending machine will dispense a sticker.
If failed somewhere along the way it will not dispense and show the user an error message.

### Connecting the components:
 <br>

| TTGO\/ESP32 | NFC Module |
| :--- | :---|
| Pin 21 | Pin SDA |
| Pin 22 | Pin SCL |
| Pin  G | Pin GND |
| Pin 3V | Pin VCC |

 <br>

| TTGO\/ESP32 | Touch Wire |
| :--- | :---|
| Pin 33 | Wire end |

 <br>


| TTGO\/ESP32 | Servo |
| :--- | :---|
| Pin 27 | Pin Pulse |
| Pin G | Pin GND |
| Pin 5V | Pin VCC |

![Image of DIY Vending Machine Scheme](/docs/DIY%20Vending%20Machine%20NFC%20TTGO_schem.png)
￼
### Which can be connected with dupont cables like this:

![Image of DIY Vending Machine Scheme](/docs/DIY%20Vending%20Machine%20NFC%20TTGO_bb.png)	

