#include <Arduino.h>
#include <BleKeyboard.h>
#include "MyShortcut.h"

MyShortcut::MyShortcut(BleKeyboard* _bleKeyboard, int caseId){
  bleKeyboard = _bleKeyboard;
  shortcutId = caseId;
}

void MyShortcut::RelaseAllkey(){
  bleKeyboard->releaseAll();
}

void MyShortcut::Action(){
  switch(shortcutId){
    //Move Window
    case 0:  
      bleKeyboard->press(KEY_LEFT_GUI);
      bleKeyboard->press(KEY_LEFT_SHIFT);
      bleKeyboard->press(KEY_RIGHT_ARROW);
    break;

     // Headphones
    case 1:  
      bleKeyboard->press(KEY_F16);
    break;

     // Speakers
    case 2:  
      bleKeyboard->press(KEY_F17);
    break;

    // Night
    case 3:  
      bleKeyboard->press(KEY_F18);
    break;

     // Play/Pause
    case 4:  
      bleKeyboard->write(KEY_MEDIA_PLAY_PAUSE);
    break;

    // Media Previous
    case 5:  
      bleKeyboard->write(KEY_MEDIA_PREVIOUS_TRACK);
    break;

    // Media Next
    case 6:  
      bleKeyboard->write(KEY_MEDIA_NEXT_TRACK);
    break;

    // Mute
    case 7:  
      bleKeyboard->write(KEY_MEDIA_MUTE);
    break;

    // TEST 1
    case 8:  
      bleKeyboard->press(KEY_NUM_1);
    break;

    // TEST 2
    case 9:  
      bleKeyboard->press(KEY_NUM_2);
    break;

    // TEST 3
    case 10:  
      bleKeyboard->press(KEY_NUM_3);
    break;

    // TEST 4
    case 11:  
      bleKeyboard->press(KEY_NUM_4);
    break;

    default:
    break;
  }
}