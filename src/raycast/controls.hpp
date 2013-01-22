#ifndef CONTROLS_HPP
#define CONTROLS_HPP

#include <iostream>
#include <map>
#include <vector>
#include <functional>

using namespace std;

namespace controls{
	#define MAX_KEYS 256

	int mouse_x = 0;
	int mouse_y = 0;

	bool gKeys[MAX_KEYS];
	typedef std::map<char,function<void()> > lambdamap;
	lambdamap keyPressListeners;
	lambdamap keyReleaseListeners;
	lambdamap keyDownListeners;
	lambdamap keyUpListeners;

	std::vector<function<void(int x, int y)> > mouseActiveMoveListeners;
	std::vector<function<void(int x, int y)> > mousePassiveMoveListeners;
	std::vector<function<void(int button, int x, int y)> > mousePressListeners;
	std::vector<function<void(int button, int x, int y)> > mouseReleaseListeners;
	std::vector<function<void()> > enterFrameListeners;

	void mousePressHandler(int button, int state, int x, int y){
		if (state == GLUT_DOWN){
			for(auto listener:mousePressListeners)
				listener(button, x, y);
		} else {
			for(auto listener:mouseReleaseListeners)
				listener(button, x, y);
		}
	}

	void mouseActiveMoveHandler(int x, int y){
		for(auto listener:mouseActiveMoveListeners)
			listener(x, y);
	}

	void mousePassiveMoveHandler(int x, int y){
		for(auto listener:mousePassiveMoveListeners)
			listener(x, y);
	}

	void keyDownHandler(unsigned char key, int x, int y){
		gKeys[key] = true;
		if (keyPressListeners.find( key ) != keyPressListeners.end()){
			keyPressListeners[key]();
		}
		if (keyDownListeners.find( key ) != keyDownListeners.end()){
			keyDownListeners[key]();
		}
	}

	void enterFrame(){
		for(auto listener:enterFrameListeners)
			listener();
	}

	void processKeys(){
		for (int i = 0; i < 256; i++){
			if (gKeys[i]){
				if (keyDownListeners.find( i ) != keyDownListeners.end()){
					keyDownListeners[i]();
				}
			} else {
				if (keyUpListeners.find( i ) != keyUpListeners.end()){
					keyUpListeners[i]();
				}
			}
		}
	}

	void processMouse(){
			for (int i = 0; i < 256; i++){
				if (gKeys[i]){
					if (keyDownListeners.find( i ) != keyDownListeners.end()){
						keyDownListeners[i]();
					}
				} else {
					if (keyUpListeners.find( i ) != keyUpListeners.end()){
						keyUpListeners[i]();
					}
				}
			}
		}

	void keyUpHandler(unsigned char key, int x, int y){
		gKeys[key] = false;
		if (keyReleaseListeners.find( key ) != keyReleaseListeners.end()){
			keyReleaseListeners[key]();
		}
		if (keyUpListeners.find( key ) != keyUpListeners.end()){
			keyUpListeners[key]();
		}
	}

	void idle(){
		processKeys();
		processMouse();
	}

	void updateMousePosition(int x, int y){
		mouse_x=x;
		mouse_y=y;
	}

	template<typename F>
	void onKeyDown(unsigned char key, F lambda){
		keyDownListeners[key] = lambda;
	}

	template<typename F>
	void onKeyUp(unsigned char key, F lambda){
		keyUpListeners[key] = lambda;
	}

	template<typename F>
	void onKeyPress(unsigned char key, F lambda){
		keyPressListeners[key] = lambda;
	}

	template<typename F>
	void onKeyRelease(unsigned char key, F lambda){
		keyReleaseListeners[key] = lambda;
	}

	template<typename F>
	void onMouseMove(bool active, F lambda){
		if(active)
			mouseActiveMoveListeners.push_back(lambda);
		else
			mousePassiveMoveListeners.push_back(lambda);
	}

	template<typename F>
	void onMousePress(F lambda){
		mousePressListeners.push_back(lambda);
	}

	template<typename F>
	void onMouseRelease(F lambda){
		mouseReleaseListeners.push_back(lambda);
	}

	template<typename F>
	void onEnterFrame(F lambda){
		enterFrameListeners.push_back(lambda);
	}

	void init(){
		glutKeyboardFunc(keyDownHandler);
		glutKeyboardUpFunc(keyUpHandler);
		glutMouseFunc(mousePressHandler);
		glutMotionFunc(mouseActiveMoveHandler);
		glutPassiveMotionFunc(mousePassiveMoveHandler);
		onMouseMove(true, [](int x, int y){
			mouse_x = x;
			mouse_y = y;
		});
		onMouseMove(false, [](int x, int y){
			mouse_x = x;
			mouse_y = y;
		});
	}

}
#endif
