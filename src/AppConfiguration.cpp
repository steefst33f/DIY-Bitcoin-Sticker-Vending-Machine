#include <AppConfiguration.h>

#define SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL 1
#if SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL
#define MY_APPCONFIGURATION_DEBUG_SERIAL Serial
#endif

AppConfiguration::AppConfiguration() : _x_position(0), _server(80), _charcount(0) {}

AppConfiguration::~AppConfiguration() {
	Serial.println("ERROR: AppConfiguration Singleton instance destroyed!!");
}

void AppConfiguration::begin() {
	Serial.begin(115200);
	Serial.println("BEGIN APPCONFIGURATION");
	for(int i=0; i<10; i++){
	  if(Serial.available()){
	    break;
	  }
	  delay(1000);
	}
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	
	//TODO: Try to connect here if credentials are available

	blink_led(100, 5); 
}

void AppConfiguration::loop() {
	Serial.println("loop");


	if (WiFi.status() == WL_CONNECTED) {
		handle_request();
	}

	if (Serial.available() > 0) {
		#if SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL
		MY_APPCONFIGURATION_DEBUG_SERIAL.println("Reading from Serial...");
		#endif
		uint8_t b = Serial.read();

		if (parse_improv_serial_byte(_x_position, b, _x_buffer, onCommandCallback, onErrorCallback)) {
			_x_buffer[_x_position++] = b;      
		} else {
			_x_position = 0;
		}
		Serial.println("_x_position: " + String(_x_position));

	}
	Serial.println("loop end");
}

void AppConfiguration::set_state(improv::State state) {  
	Serial.println("set_state: " + String(state));

	std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
	data.resize(11);
	data[6] = improv::IMPROV_SERIAL_VERSION;
	data[7] = improv::TYPE_CURRENT_STATE;
	data[8] = 1;
	data[9] = state;

	uint8_t checksum = 0x00;
	for (uint8_t d : data)
	checksum += d;
	data[10] = checksum;

	Serial.write(data.data(), data.size());
}


void AppConfiguration::send_response(std::vector<uint8_t> &response) {
	std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
	data.resize(9);
	data[6] = improv::IMPROV_SERIAL_VERSION;
	data[7] = improv::TYPE_RPC_RESPONSE;
	data[8] = response.size();
	data.insert(data.end(), response.begin(), response.end());

	uint8_t checksum = 0x00;
	for (uint8_t d : data)
	checksum += d;
	data.push_back(checksum);

	Serial.write(data.data(), data.size());
}

void AppConfiguration::set_error(improv::Error error) {
	std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
	data.resize(11);
	data[6] = improv::IMPROV_SERIAL_VERSION;
	data[7] = improv::TYPE_ERROR_STATE;
	data[8] = 1;
	data[9] = error;

	uint8_t checksum = 0x00;
	for (uint8_t d : data)
	checksum += d;
	data[10] = checksum;

	Serial.write(data.data(), data.size());
}

void AppConfiguration::onErrorCallback(improv::Error err) {
	getInstance().blink_led(2000, 3);
	#if SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL
	MY_APPCONFIGURATION_DEBUG_SERIAL.println("Improv configuration ERROR: " + String(err));
	#endif
}

bool AppConfiguration::onCommandCallback(improv::ImprovCommand cmd) {

	switch (cmd.command) {
	case improv::Command::GET_CURRENT_STATE:
	{
		#if SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL
		MY_APPCONFIGURATION_DEBUG_SERIAL.println("IMPROV Recieved Cmd: GET_CURRENT_STATE");
		#endif
		if ((WiFi.status() == WL_CONNECTED)) {
		set_state(improv::State::STATE_PROVISIONED);
		std::vector<uint8_t> data = improv::build_rpc_response(improv::GET_CURRENT_STATE, getLocalUrl(), false);
		send_response(data);

		} else {
		set_state(improv::State::STATE_AUTHORIZED);
		}
		
		break;
	}

	case improv::Command::WIFI_SETTINGS:
	{
		#if SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL
		MY_APPCONFIGURATION_DEBUG_SERIAL.println("IMPROV Recieved Cmd: WIFI_SETTINGS");
		#endif
		if (cmd.ssid.length() == 0) {
		set_error(improv::Error::ERROR_INVALID_RPC);
		break;
		}
	 
		set_state(improv::STATE_PROVISIONING);
		
		if (getInstance().connectWifi(cmd.ssid, cmd.password)) {

		getInstance().blink_led(100, 3);
		
		//TODO: Persist credentials here

		set_state(improv::STATE_PROVISIONED);        
		std::vector<uint8_t> data = improv::build_rpc_response(improv::WIFI_SETTINGS, getLocalUrl(), false);
		send_response(data);
		getInstance()._server.begin();

		} else {
		set_state(improv::STATE_STOPPED);
		set_error(improv::Error::ERROR_UNABLE_TO_CONNECT);
		}
		
		break;
	}

	case improv::Command::GET_DEVICE_INFO:
	{
		#if SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL
		MY_APPCONFIGURATION_DEBUG_SERIAL.println("IMPROV Recieved Cmd: GET_DEVICE_INFO");
		#endif
		std::vector<std::string> infos = {
		// Firmware name
		"ImprovWiFiDemo",
		// Firmware version
		"1.0.0",
		// Hardware chip/variant
		"ESP32",
		// Device name
		"SimpleWebServer"
		};
		std::vector<uint8_t> data = improv::build_rpc_response(improv::GET_DEVICE_INFO, infos, false);
		send_response(data);
		break;
	}

	case improv::Command::GET_WIFI_NETWORKS:
	{
		#if SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL
		MY_APPCONFIGURATION_DEBUG_SERIAL.println("IMPROV Recieved Cmd: GET_WIFI_NETWORKS");
		#endif
		getAvailableWifiNetworks();
		break;
	}

	default: {
		#if SHOW_MY_APPCONFIGURATION_DEBUG_SERIAL
		MY_APPCONFIGURATION_DEBUG_SERIAL.println("IMPROV Recieved Cmd: ERROR_UNKNOWN_RPC");
		#endif
		set_error(improv::ERROR_UNKNOWN_RPC);
		return false;
	}
	}

	return true;
}

std::vector<std::string> AppConfiguration::getLocalUrl() {
	return {
	// URL where user can finish onboarding or use device
	// Recommended to use website hosted by device
	String("http://" + WiFi.localIP().toString()).c_str()
	};
}

void AppConfiguration::getAvailableWifiNetworks() {
	int networkNum = WiFi.scanNetworks();

	for (int id = 0; id < networkNum; ++id) { 
	std::vector<uint8_t> data = improv::build_rpc_response(
			improv::GET_WIFI_NETWORKS, {WiFi.SSID(id), String(WiFi.RSSI(id)), (WiFi.encryptionType(id) == WIFI_AUTH_OPEN ? "NO" : "YES")}, false);
	send_response(data);
	delay(1);
	}
	//final response
	std::vector<uint8_t> data =
			improv::build_rpc_response(improv::GET_WIFI_NETWORKS, std::vector<std::string>{}, false);
	send_response(data);
}




///SEVER CLient

// *** Web Server
void AppConfiguration::handle_request() {

	WiFiClient client = _server.available();
	if (client) 
	{
	memset(_linebuf,0,sizeof(_linebuf));
	_charcount=0;
	// an http request ends with a blank line
	boolean currentLineIsBlank = true;
	while (client.connected()) 
	{
		if (client.available()) 
		{
		char c = client.read();
		//read char by char HTTP request
		_linebuf[_charcount]=c;
		if (_charcount<sizeof(_linebuf)-1) _charcount++;

		if (c == '\n' && currentLineIsBlank) 
		{
			client.println("HTTP/1.1 200 OK");
			client.println("Content-Type: text/html");
			client.println("Connection: close");  // the connection will be closed after completion of the response
			client.println();
			client.println("<!DOCTYPE HTML><html><body>");
			client.println("<h1>Welcome! </h1>");
			client.println("<p>This is a simple webpage served by your ESP32</p>");
			client.println("<h3>Chip Info</h3>");
			client.println("<ul><li>Model:");
			client.println(ESP.getChipModel());
			client.println("</li><li>Cores: ");
			client.println(ESP.getChipCores());
			client.println("</li><li>Revision: ");
			client.println(ESP.getChipRevision());
			client.println("</li></ul>");
			client.println("<h3>Network Info</h3>");
			client.println("<ul><li>SSID: ");
			client.println(WiFi.SSID());
			client.println("</li><li>IP Address: ");
			client.println(WiFi.localIP());
			client.println("</li><li>MAC Address: ");
			client.println(WiFi.macAddress());
			client.println("</li></ul>");          
			client.println("</body></html>");
			break;
		}
		}
	}
	delay(1);
	client.stop();
	}  
}

void AppConfiguration::blink_led(int d, int times) {
	for (int j=0; j<times; j++){
	digitalWrite(LED_BUILTIN, HIGH);
	delay(d);
	digitalWrite(LED_BUILTIN, LOW);
	delay(d);
	}
	
}

bool AppConfiguration::connectWifi(std::string ssid, std::string password) {
	uint8_t count = 0;

	WiFi.begin(ssid.c_str(), password.c_str());

	while (WiFi.status() != WL_CONNECTED) {
	blink_led(500, 1);

	if (count > MAX_ATTEMPTS_WIFI_CONNECTION) {
		WiFi.disconnect();
		return false;
	}
	count++;
	}

	return true;
}


