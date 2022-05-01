    #include<ESP8266WiFi.h>    //ESP8266 WIFI连接库
    #include<ArduinoJson.h>    //数据分析库
    #include <Arduino.h>
    #include <U8g2lib.h>       //LED显示库
    #ifdef U8X8_HAVE_HW_SPI
    #include <SPI.h>
    #endif
    #ifdef U8X8_HAVE_HW_I2C
    #include <Wire.h>
    #endif
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
    
    #define SUN 0              //天气符号定义
    #define SUN_CLOUD  1
    #define CLOUD 2
    #define RAIN 3
    #define THUNDER 4
    #define WU 5
    
    //将天气转换为对应的符号
     void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
    {
      // 具体编码参考: https://github.com/olikraus/u8g2/wiki/fntgrpiconic
      switch(symbol)
      {
        case SUN:
          u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
          u8g2.drawGlyph(x, y, 69);
          break;
        case SUN_CLOUD:
          u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
          u8g2.drawGlyph(x, y, 65);
          break;
        case CLOUD:
          u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
          u8g2.drawGlyph(x, y, 64);
          break;
        case RAIN:
          u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
          u8g2.drawGlyph(x, y, 67);
          break;
        case THUNDER:
          u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t);
          u8g2.drawGlyph(x, y, 67);
          break;
        case WU:
          u8g2.setFont(u8g2_font_open_iconic_text_6x_t);
          u8g2.drawGlyph(x, y, 78);
          break;
      }
    }

    //绘制天气符号和温度函数
    void drawWeather(uint8_t symbol, int degree)
    {
      //绘制天气符号
      drawWeatherSymbol(0, 48, symbol);
      //绘制温度
      u8g2.setFont(u8g2_font_logisoso32_tf);
      u8g2.setCursor(48+3, 42);
      u8g2.print(degree);
      u8g2.print("°C");       // requires enableUTF8Print()
    }

    //LED初始化
    void drawScrollString(int16_t offset, const char *s)
    {
      static char buf[36];    // should for screen with up to 256 pixel width
      size_t len;
      size_t char_offset = 0;
      u8g2_uint_t dx = 0;
      size_t visible = 0;
      u8g2.setDrawColor(0);   // clear the scrolling area
      u8g2.drawBox(0, 49, u8g2.getDisplayWidth()-1, u8g2.getDisplayHeight()-1);
      u8g2.setDrawColor(1);   // set the color for the text
      len = strlen(s);
      if ( offset < 0 )
      {
        char_offset = (-offset)/8;
        dx = offset + char_offset*8;
        if ( char_offset >= u8g2.getDisplayWidth()/8 )
          return;
        visible = u8g2.getDisplayWidth()/8-char_offset+1;
        strncpy(buf, s, visible);
        buf[visible] = '\0';
        u8g2.setFont(u8g2_font_8x13_mf);
        u8g2.drawStr(char_offset*8-dx, 62, buf);
      }
      else
      {
        char_offset = offset / 8;
        if ( char_offset >= len )
          return; // nothing visible
        dx = offset - char_offset*8;
        visible = len - char_offset;
        if ( visible > u8g2.getDisplayWidth()/8+1 )
          visible = u8g2.getDisplayWidth()/8+1;
        strncpy(buf, s+char_offset, visible);
        buf[visible] = '\0';
        u8g2.setFont(u8g2_font_8x13_mf);
        u8g2.drawStr(-dx, 62, buf);
      }
    }

    //LED滚屏
    void draw(const char *s, uint8_t symbol, int degree)
    {
      int16_t offset = -(int16_t)u8g2.getDisplayWidth();
      int16_t len = strlen(s);
      u8g2.clearBuffer();                     // clear the internal memory
      drawWeather(symbol, degree);            // draw the icon and degree only once
      for(;;)                                 // then do the scrolling
      {
        drawScrollString(offset, s);          // no clearBuffer required, screen will be partially cleared here
        u8g2.sendBuffer();                    // transfer internal memory to the display
        delay(20);
        offset+=2;
        if ( offset > len*8+1 )
          break;
      }
    }
    
    const char* ssid ="???";                   //输入热点名称
    const char* password ="???";               //输入热点密码
    //const char* ssid ="???";                 //输入热点名称
    //const char* password ="???";             //输入热点密码
    const char* host ="api.seniverse.com";     //心知天气网站
    const char* APIKEY ="???";                 //输入自己申请的知心天气私钥
    const char* city ="???";                   //可根据需要改为其余城市的拼音
    const char* language ="zh-Hans";           //设置语言为中文

    const unsigned long BAUD_RATE=115200;      
    const unsigned long HTTP_TIMEOUT=5000;
    const size_t MAX_CONTENT_SIZE=1000;

    struct WeatherData{                             //存储天气数据的结构体，可根据需要自行添加成员变量
      char city[16];
      char weather[32];
      char temp[16];
      char udate[32];
    };
    
    char *ab;
    char *tmp;
    int tmpp;
    int w=26;

    WiFiClient client;//创建了一个网络对象
    char response[MAX_CONTENT_SIZE];
    char endOfHeaders[]="\r\n\r\n";

    void setup() {
      delay(500);
      Serial.begin(BAUD_RATE);
      //wifiConnect();          //连接WiFi
      //client.setTimeout(HTTP_TIMEOUT);
      pinMode(D8,OUTPUT);       //白灯
      pinMode(D7,OUTPUT);       //雾化片
      pinMode(D9,OUTPUT);       //水泵
      pinMode(D10,OUTPUT);      //蓝灯
      u8g2.begin();
      u8g2.enableUTF8Print();   // enable UTF8 support for the Arduino print() function
   }

    void loop() {
      digitalWrite(D9,HIGH);
      digitalWrite(D7,HIGH);

      //WIFI连接部分
      while(!client.connected()){
       if(!client.connect(host,80)){//尝试建立连接
          Serial.println("connection....");
          delay(500);
        }
     }
      //连接成功，发送GET请求
      if(sendRequest(host,city,APIKEY)&&skipResponseHeaders()){ //发送http请求 并且跳过响应头
        clrEsp8266ResponseBuffer();                             //清除缓存
        readReponseContent(response,sizeof(response));          //从HTTP服务器响应中读取正文
        WeatherData weatherData;
        if(parseUserData(response,&weatherData)){               //判断Json数据包是否分析成功
            printUserData(&weatherData);                        //输出读取到的天气信息
        }
      }
      stopConnect();
      delay(5000);
      

      //ab="雨";
      //以下语句根据ab的不同值（即天气状况）控制LED输出
      Serial.println(ab);
      if(strcmp(ab,"雾")==0) {
        analogWrite(D8,120);
        draw("I can't see anything!", WU, w);
        digitalWrite(D7,LOW);
        delay(40000);
        digitalWrite(D7,HIGH);
        analogWrite(D8,0);
      }
      
      if(strcmp(ab,"雷雨")==0){
        digitalWrite(D10,HIGH);
        draw("That sounds like thunder.", THUNDER, w);
        for(int i=0;i<5;i++){
          draw("That sounds like thunder.", THUNDER, w);
            digitalWrite(D9,LOW);
            delay(4000);
            digitalWrite(D9,HIGH);
            delay(1000);
            digitalWrite(D10,HIGH);
            delay(100);
            digitalWrite(D10,LOW);
            delay(100);
            digitalWrite(D10,HIGH);
            delay(100);
            digitalWrite(D10,LOW);
            delay(100);
            digitalWrite(D10,HIGH);
            delay(2000);
        }
        digitalWrite(D10,LOW);
      }
            

      if(strcmp(ab,"雨")==0){
        draw("It's raining cats and dogs.", RAIN, w);
          digitalWrite(D9,LOW);
          delay(4000);
          digitalWrite(D9,HIGH);
          delay(5000);
        for(int i=0;i<5;i++){
          draw("It's raining cats and dogs.", RAIN, w);
          digitalWrite(D9,LOW);
          delay(4000);
          digitalWrite(D9,HIGH);
          delay(5000);
        }
      }

      if(strcmp(ab,"阴")==0) {
        analogWrite(D8,120); 
        for(int j=0;j<5;j++){
        draw("It's stopped raining", CLOUD, w);
        delay(5000);
        }
        analogWrite(D8,0); 
      }

        if(strcmp(ab,"多云")==0) {
        analogWrite(D8,0); 
        for(int j=0;j<5;j++){
        
          for(int k=0;k<255;k++){
          analogWrite(D8,k);
          delay(8);}
          draw("The sun's come out!", SUN_CLOUD, w);
          for(int u=255;u>0;u--){
            analogWrite(D8,u);
            delay(8);
          }
        }
        analogWrite(D8,0);
      }

      if(strcmp(ab,"晴")==0) {
        digitalWrite(D8,HIGH); 
        for(int j=0;j<5;j++){
        draw("What a beautiful day!", SUN, w);
        delay(5000);
        }
        digitalWrite(D8,LOW);
      }
    }

    //WIFI连接函数
    void wifiConnect(){
      WiFi.mode(WIFI_STA);                     //设置esp8266工作模式
      Serial.print("Connecting to");
      Serial.println(ssid);
      WiFi.begin(ssid,password);               //连接WiFi
      WiFi.setAutoConnect(true);
      while(WiFi.status()!=WL_CONNECTED){      //该函数返回WiFi的连接状态
        delay(500);
        Serial.print(".");
      }
      Serial.println("");
      Serial.println("WiFi connected");
      delay(500);
      Serial.println("IP address:");
      Serial.println(WiFi.localIP());
    }

    //以下函数功能为从心知天气网上拉取数据并输出到串口监视器
    bool sendRequest(const char* host,const char* cityid,const char* apiKey){
      String GetUrl="/v3/weather/now.json?key=";
      GetUrl+=APIKEY;
      GetUrl+="&location=";
      GetUrl+=city;
      GetUrl+="&language=";
      GetUrl+=language;
      GetUrl+="&unit=c ";
      client.print(String("GET ")+GetUrl+"HTTP/1.1\r\n"+"Host:"+host+"\r\n"+"Connection:close\r\n\r\n");
      Serial.println("creat a request:");
      Serial.println(String("GET ")+GetUrl+"HTTP/1.1\r\n"+"Host:"+host+"\r\n"+"Connection:close\r\n\r\n");
      delay(1000);
      return true;
    }

    bool skipResponseHeaders(){
      bool ok=client.find(endOfHeaders);
      if(!ok){
        Serial.println("No response of invalid response!");
      }
      return ok;
    }

    void readReponseContent(char* content,size_t maxSize){
      size_t length=client.readBytes(content,maxSize);
      delay(100);
      Serial.println("Get the data from Internet");
      content[length]=0;
      Serial.println(content);//输出读取到的数据
      Serial.println("Read data Over!");
      client.flush();//刷新客户端
    }

    bool parseUserData(char* content,struct WeatherData* weatherData){
      DynamicJsonBuffer jsonBuffer;//创建一个动态缓冲区实例
      JsonObject&root=jsonBuffer.parseObject(content);//根据需要解析的数据来计算缓冲区的大小
      if(!root.success()){
        Serial.println("JSON parsing failed!");
        return false;
      }
      strcpy(weatherData->city,root["results"][0]["location"]["name"]);
      strcpy(weatherData->weather,root["results"][0]["now"]["text"]);
      strcpy(weatherData->temp,root["results"][0]["now"]["temperature"]);
      strcpy(weatherData->udate,root["results"][0]["last_update"]);
      ab=(weatherData->weather);
      tmp=(weatherData->temp);
      Serial.println("att");
      w=(tmp[0]-48)*10+tmp[1]-48;
      Serial.println(w);
      Serial.println(tmp);
      return true;
    }

    void printUserData(const struct WeatherData* weatherData){
      /*Serial.println("Print parsed data:");
      Serial.print("City:");
      Serial.print(weatherData->city);
      Serial.print("  Weather:");
      Serial.print(weatherData->weather);
      Serial.print("  Temp:");
      Serial.print(weatherData->temp);
      Serial.print("℃");
      Serial.print("  UpdateTime:");
      Serial.println(weatherData->udate);*/
      Serial.println(weatherData->weather);
      Serial.println(ab);
    }

    void stopConnect(){
      Serial.println("Disconnect");
      client.stop();
    }

    void clrEsp8266ResponseBuffer(void){
      memset(response,0,MAX_CONTENT_SIZE);
    }
