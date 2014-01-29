#define RENEW_INTERVAL      20*1000      // 30 secs
#define RETRY_INTERVAL      10*1000      // 10 secs
#define RESPONSE_INTERVAL   1*1000       // 1 sec

TCPClient client;
byte server[] = { 192, 168, 1, 100 }; // Google

// IO
int led = D0; 
int waitpin_C = D7; 
int waitpin_NO = D5; 
int waitpin_NC = D3; 
int displayReset = A0;
int debug_tx = D0;
int debug_tx_S = D1; 

// Globals
 int state = 0;
 int wait = 0;
 int loopwait = 10;
 int displaywait = 100;
 int tries = 0;
 int store = 0;
 int hash = 0;
float temperature = 0;

#define COMMAND_SIZE 32
 char command[COMMAND_SIZE];
int command_i=0;

// Displays float with one decimal
int display(float fx){
    int ix;
    for(int i=0; i < COMMAND_SIZE;i++) command[i] = 0;
    // Lets see if we can display this number
    if(fx < -999.0 || fx > 999.0)
        return -1;  // We cannot
    
    // Round
    ix = abs(fx*10);
    
    // Write display
    unsigned int i,ii=3;
    for(i=1;i<1000;i*=10){			
        command[ii--] = (ix/i)%10;
        if(ii<0)
            break;
    } 
    if(fx<0)    command[0] = '-';
    
    //sprintf(&command[0], "%4d", ix); //Convert deciSecond into a string that is right adjusted
    // Enable dot
    command[4] = 0x77;
    command[5] = 0x04;
    
    for(int i=0; i<6; i++){
        Serial1.write(command[i]);
        delay(1);
    }

    return 0;
    
}

void setup()
{
    Serial1.begin(9600);
    
    Serial1.println("Thermometer sketch");
    
    pinMode(led, OUTPUT);  
    // Button resistorless
    pinMode(waitpin_C, INPUT );  
    pinMode(waitpin_NO, OUTPUT );  
    pinMode(waitpin_NC, OUTPUT );
    digitalWrite(waitpin_NO, LOW);
    digitalWrite(waitpin_NC, HIGH);
    
    // Debug tx
    pinMode(debug_tx, INPUT_PULLDOWN  );  
    pinMode(debug_tx_S, OUTPUT );
    digitalWrite(debug_tx_S, HIGH);
    
    // Display reset
    pinMode(displayReset, OUTPUT);
    digitalWrite(displayReset, LOW);
    delay(10);
    digitalWrite(displayReset, HIGH);
    
    // Api
    Spark.variable("temperature", &temperature, INT);
    
    state = 0;
    wait = RETRY_INTERVAL;

    // Connecting
}

void loop()
{
    int debug = digitalRead(debug_tx);
    
    if(!digitalRead(waitpin_C)){
        // Skip mode, to ensure bootloader stays available
        char buffer[32];
        sprintf(buffer,"skip state: %d\r\n",state);
        Serial1.print(buffer);
        digitalWrite(led, HIGH);   // Turn ON the LED       
        delay(100);
        return;
    }else{
        digitalWrite(led, LOW);   // Turn OFF the LED  
    }

    delay(1);
    switch(state){
        case 0:
            // Waiting for next time
            wait-=10;
            if(wait<0){
                wait = 0;
                state = 1;
            }
            break;
        case 1:
            // Connecting
            if(debug) Serial1.println("connecting");
            if (client.connect(server, 80)){
                if(debug) Serial1.println("connected");
                state = 2;
            }else{
                if(debug) Serial1.println("connection failed state 1");
                wait = RETRY_INTERVAL;
                state = 0;
            }
            break;
        case 2:
            // Requesting
            tries = 0;
            store = 0;
            hash = 0;
            command_i = 0;
            for(int i=0; i<COMMAND_SIZE; i++) command[i] = 0;
            if(client.connected()){
                if(debug) client.println("GET // HTTP/1.0\r\n\r\n");
                wait = RESPONSE_INTERVAL;
                state = 3;
            }else{
                if(debug) Serial1.println("connection lost state 2");
                wait = RETRY_INTERVAL;
                state = 0;
            }
            break;
        case 3:
            // Receiving
            if(client.connected()){
                if (client.available() > 0) 
                {
                    // Print response to serial
                    char c = client.read();
                    if(debug) Serial1.print(c);
                    
                    // If last expected char found, quit reading
                    if(c =='>') hash = 1;
                    // If first char of data found, start storing the string
                    if(c =='<') store = 1;
                    if(store){
                        command[command_i++] = c;
                    }
                }else{
                    if(debug) Serial1.println("nd s3 ");  
                    delay(100);
                }
                // Quit reading
                if(hash){
                    Serial1.println();
                    state = 4;
                }
            }else{
                // We lost connection
                if(debug) Serial1.println("connection lost state 3");
                wait = RETRY_INTERVAL;
                state = 0;
            }
            break;
        case 4:
            // Disconnecting
            if(client.connected()){
                client.stop();
                if(debug) Serial1.println("connection closed state 4");
                wait = RENEW_INTERVAL;
                state = 5;
            }else{
                if(debug) Serial1.println("connection closed by server state 4");
                wait = RENEW_INTERVAL;
                state = 5; 
            }
            break;
        case 5:
        {
            int code = 0;
            // Parse data read
            client.stop();
            if(debug) Serial1.println("I've got this:");
            for(int i=0; i<COMMAND_SIZE; i++) Serial1.write(command[i]);
            Serial1.println("");
            
            // Control display
            //int code = (command[1]*1000)+(command[2]*100)+(command[3]*10)+(command[4]*1);
            //float temp = atoi((const char*)&command[6]) + (0.1 * atoi((const char*)&command[8]));
            sscanf((const char*)command,"<%d;%f>",&code,&temperature);
            
            if(debug) Serial1.print("Code: ");Serial1.println(code);
            if(debug) Serial1.print("Temp: ");Serial1.println(temperature);
  
            display(temperature);
        
            
            /*
            char tempString[10]; //Used for sprintf
            Serial1.println();
            Serial1.write('v');
            sprintf(tempString, "%4d", temp); //Convert deciSecond into a string that is right adjusted
            Serial1.print(tempString);
            */
            wait = RENEW_INTERVAL;
            state = 0;
        }
            break;
            
    }
}
