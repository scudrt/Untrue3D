#include "InputHandler.h"
#include "graphics.h"
#include "UT3D.h"

using namespace untrue;

bool InputHandler::needExit = false;

//处理输入，影响相机漫游
void InputHandler::handle(float deltaTime) {
	UT3D* ut = UT3D::instance();

	static bool isClicked = false,
		movingLeft = false,
		movingRight = false,
		movingForward = false,
		movingBack = false;
	static bool *movePtr = nullptr;
	static mouse_msg mouse, nowmouse;
	static key_msg key;
	static float inten = 10.0f;
	const float moveSpeed = 350.0f,
		rotateSpeed = 0.15f;

	//delay的过程可能会有多个键鼠事件出现，因此应该当帧一次性处理完
	while (mousemsg()) {
		nowmouse = getmouse();
		if (nowmouse.is_down()) {
			isClicked = true;
			mouse = nowmouse;
		} else if (nowmouse.is_up()) {
			isClicked = false;
		}

		if (isClicked) {
			if (nowmouse.is_move()) {
				//图形库的屏幕Y轴与本人定义的屏幕Y轴方向相反
				ut->mainCamera->rotateBy(vec3(
					mouse.y - nowmouse.y,
					0.0,
					mouse.x - nowmouse.x
				) * rotateSpeed);
				mouse = nowmouse;
			}
		}
	}
	while (kbmsg()) {
		key = getkey();
		if (key.key == 'A') {
			movingRight = false;
			movePtr = &movingLeft;
		} else if (key.key == 'D') {
			movePtr = &movingRight;
			movingLeft = false;
		} else if (key.key == 'S') {
			movePtr = &movingBack;
			movingForward = false;
		} else if (key.key == 'W') {
			movePtr = &movingForward;
			movingBack = false;
		} else if (key.key == '=') {
			inten += 0.4f;
			ut->lightings[0]->setIntensity(inten);
			continue;
		} else if (key.key == '-') {
			inten -= 0.4f;
			ut->lightings[0]->setIntensity(inten);
			continue;
		} else if (key.key == key_esc) { //exit
			needExit = true;
			continue; //skip moving signal
		}else { //skip the not defined keys
			continue;
		}
		if (key.msg == key_msg_down) {
			*movePtr = true;
		} else if (key.msg == key_msg_up) {
			*movePtr = false;
		}
	}

	if (movingLeft) {
		ut->mainCamera->translateBy(vec3(-moveSpeed, 0, 0) * deltaTime);
	} else if (movingRight) {
		ut->mainCamera->translateBy(vec3(moveSpeed, 0, 0) * deltaTime);
	}
	if (movingBack) {
		ut->mainCamera->translateBy(vec3(0, -moveSpeed, 0) * deltaTime);
	} else if (movingForward) {
		ut->mainCamera->translateBy(vec3(0, moveSpeed, 0) * deltaTime);
	}
}
