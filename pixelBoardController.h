#ifndef PIXEL_BOARD_CONTROLLER_H	
#define PIXEL_BOARD_CONTROLLER_H
#include <WebSocketsServer.h>
#define NUM_BTNS 7
#define UP 0 
#define DOWN 1
#define LEFT 2
#define RIGHT 3
#define BTNS 4
#define BTNA 5
#define BTNB 6



class PixelBoardController{
	private:
		WebSocketsServer* webSocket;
		unsigned long startRead;
		
		uint8_t btnStatus[NUM_BTNS];
		uint8_t stickyBtnStatus[NUM_BTNS];
		int8_t lastKey = -1;
		bool getKeyFor(uint8_t key, bool getSticky){
			if(getSticky){
				return getStickyBtnStatus(key) == 1;
			}else{
				return getBtnStatus(key) == 1;
			}
		}
	public:
		
		PixelBoardController(){};
		PixelBoardController(WebSocketsServer* ws) : webSocket(ws){};
		
		void broadcastTXT(const char* msg){
			webSocket->broadcastTXT(msg);
		}
		void begin(){
			for(int i = 0; i < NUM_BTNS; ++i){
				btnStatus[i] = 0;
				stickyBtnStatus[i] = 0;
			}
		}
		void setBtnStatus(uint8_t btn, uint8_t status){
			btnStatus[btn] = status;
			if(status > 0){
				if(lastKey >= 0)
					stickyBtnStatus[lastKey] = 0; //Clear last key
				stickyBtnStatus[btn]  = status;
				lastKey = btn;
			}
		};
		
		void clearStickyBtns(){
			for(int i = 0; i < NUM_BTNS; ++i){
				stickyBtnStatus[i] = 0;
			}
			lastKey = -1;
		}
		uint8_t getStickyBtnStatus(uint8_t key){
			return stickyBtnStatus[key];
		}
		
		uint8_t getBtnStatus(uint8_t key){
		   return btnStatus[key];
		}
		

		bool isUpPressed(bool getSticky = true){
			return getKeyFor(UP, getSticky);
		}
		bool isDownPressed(bool getSticky = true){
			return getKeyFor(DOWN, getSticky);
		}
		bool isLeftPressed(bool getSticky = true){
			return  getKeyFor(LEFT, getSticky);
		}
		
		bool isRightPressed(bool getSticky = true){
			return  getKeyFor(RIGHT, getSticky);
		}

    bool isBTNSPressed(bool getSticky = true){
      return getKeyFor(BTNS, getSticky);
    }
    bool isBtnAPressed(bool getSticky = true){
      return getKeyFor(BTNA, getSticky);
    }
    bool isBtnBPressed(bool getSticky = true){
      return getKeyFor(BTNB, getSticky);
    }

   /* bool isBtnCPressed(bool getSticky = true){
      return getKeyFor(BTNC, getSticky);
    }
    bool isBtnDPressed(bool getSticky = true){
      return getKeyFor(BTND, getSticky);
    }
  */
    
};
#endif
