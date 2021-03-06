/*--------------------------------------------*/
/*                    5249S                   */
/*                The ManiVEXo                */
/*                    Main                    */
/*                Version 0.2.0               */
/*--------------------------------------------*/
#include "robot-config.h"//Includes config file
#include <cmath>//Includes math operations Ex: pow, sin, sqrt
    
static int mode = -1;//Mode for the robot to operate in
static bool colorRed = true;//True if on red alliance, false if on blue
void auton(int);//Declares Auton and driver functions, initialized in Driver Auton file
void driver();
static bool warning[10][2];//Array of warning statuses for warning the driver of problems with the robot
void runDiagnostics(){//Method for displaying any problems with the robot
    char warningText[10][6] = {"BatL ","BatH ","MdlH ","MdrH ","MllH ","MlrH","","","",""};
    for (int i = 0; i < 10; i++){//store the previous state of each error to check for a change
        warning[i][1] = warning[i][0];
    }
    warning[0][0] = robotMain.Battery.capacity() < 25;//Battery capacity < 25%
    warning[1][0] = robotMain.Battery.temperature() > 80;//Battery temperature > 80%
    warning[2][0] = mtrDriveLeft.temperature(vex::percentUnits::pct) > 70;//Left Drive Motor Temp >70%
    warning[3][0] = mtrDriveRight.temperature(vex::percentUnits::pct) > 70;//Right Drive Motor Temp >70%
    warning[4][0] = mtrLiftLeft.temperature(vex::percentUnits::pct) > 70;//Left Lift Motor Temp >70%
    warning[5][0] = mtrLiftRight.temperature(vex::percentUnits::pct) > 70;//Right Lift Motor Temp >70%
    warning[6][0] = false;
    warning[7][0] = false;
    warning[8][0] = false;
    warning[9][0] = false;
    
    
    bool update = false;//Checks if a value has changed and needs to be updated to comply with controller screen's slow update rate
    for (int i = 0; i < 10; i++){
        if ((warning[i][0] && !warning[i][1]) || (warning[i][1] && !warning[i][0])){//Update the display if any of the warnings have changed
            update = true;
            break;
        }
    }
    if (update) {//Display all warnings
        ctrPrimary.Screen.clearLine(2);
        ctrPrimary.Screen.clearLine(3);
        ctrPrimary.Screen.setCursor(2,0);
        for (int i = 0; i < 10; i++){
            if (warning[i][0]){
                ctrPrimary.Screen.print("%s ", warningText[i]);//Display all warning text in succession
            }
        }
    }
}

class GyroSettings {//Class used to set gyros to specific values, as they can't be changed in the program
    private:
        int gyroBias = 0;
        int reverse = 1;
    public:
        void setValues(int trueValue, int currentValue, bool rev){//Sets proper values
            reverse = rev?-1:1;
            gyroBias = currentValue - reverse * trueValue;
        }
        int value(int currentValue){//returns true value with wanted shift
            return reverse * (currentValue - gyroBias);
        }
};
void wait(int time){
    vex::task::sleep(time);
}
GyroSettings gyroLauncherSet;
GyroSettings gyroNavSet;
void calibrateGyros(){//Calibrates gyros
    robotMain.Screen.clearScreen();
    robotMain.Screen.setCursor(0,0);
    robotMain.Screen.print("Gyros Calibrating");
    robotMain.Screen.newLine();
    robotMain.Screen.print("Do Not Touch Robot!");
    robotMain.Screen.newLine();
    robotMain.Screen.print("(B) Bypass");
    gyroNav.startCalibration();
    gyroLauncher.startCalibration();
    while(gyroNav.isCalibrating() || gyroLauncher.isCalibrating()){//waits for both gyros to finish 
        if (ctrPrimary.ButtonB.pressing()){
            break;//allows bypass
        }
        wait(20);
    }
    while(ctrPrimary.ButtonB.pressing()){wait(20);}
    gyroNavSet.setValues(0, gyroNav.value(vex::rotationUnits::deg), false);
    gyroLauncherSet.setValues(0, gyroLauncher.value(vex::rotationUnits::deg), false);
    robotMain.Screen.clearScreen();
    
    
}
void stopAllMotors(){
    mtrDriveLeft.stop(vex::brakeType::coast);
    mtrDriveRight.stop(vex::brakeType::coast);
    mtrLiftLeft.stop(vex::brakeType::coast);
    mtrLiftRight.stop(vex::brakeType::coast);
    
}
bool isField(){
    return compControl.isCompetitionSwitch() || compControl.isFieldControl();
}
class DisplaySelection {//Class created to hold and change the values needed to move the display up and down
        private:
            int maxLines = 3;
            int current = 0;
            int topLine = 0;
            bool selectionMade = false;

            unsigned int max = 0;
            int getPosition(){
                return current - topLine;
            }

            void moveDown(){
                if (current != max - 1){
                    if (current == topLine + maxLines - 1){
                        topLine ++;
                    }
                    current ++;
                }
            }
            void moveUp(){
                if (current != 0){
                    if (current == topLine){
                        topLine --;
                    }
                    current --;
                } else {
                    current = max - 1;
                    topLine = max - maxLines < 0 ? 0 : max - maxLines;
                }
            }
            int update(bool select, bool up, bool down){
                if(select){
                    return current;
                }
                if(up){
                    moveUp();
                } 
                if(down){
                    moveDown();
                }
                ctrPrimary.Screen.clearScreen();
                for (int i=0; i < maxLines; i++){
                    ctrPrimary.Screen.setCursor(i+1,3);
                    ctrPrimary.Screen.print("%s", text[i + topLine]);
                }
                ctrPrimary.Screen.setCursor(getPosition()+1,0);
                ctrPrimary.Screen.print("->");
                return -1;
            }
        public:
            char text[8][32];
            DisplaySelection(unsigned int maxOptions){
                max = maxOptions;
            }
            int select(){
                while(true){//repeat update until a selection is chosen
                    bool selectBtn = ctrPrimary.ButtonA.pressing();
                    bool up = ctrPrimary.ButtonUp.pressing();
                    bool down = ctrPrimary.ButtonDown.pressing();
                    int status = update(selectBtn, up, down);//call update function
                    while(ctrPrimary.ButtonA.pressing() || ctrPrimary.ButtonUp.pressing() || ctrPrimary.ButtonDown.pressing()){wait(20);}//wait for all buttons to be released
                    if (status != -1){//repeat loop until selection is made (update return something other than -1)
                        return status;//return number selected
                        break;
                    }
                    while(!(ctrPrimary.ButtonA.pressing() || ctrPrimary.ButtonUp.pressing() || ctrPrimary.ButtonDown.pressing())){
                        if (isField()){
                            ctrPrimary.Screen.clearScreen();
                            ctrPrimary.Screen.setCursor(1,0);
                            ctrPrimary.Screen.print("Remove Field Cable");
                            while (isField()){
                                wait(20);
                            }
                            break;
                        }
                        wait(20);
                    }
                }
            }

};
bool confirmAuton(){//Confirms it is allowed to run auton
    if (mode == 0 || mode == 1){//If in field control or skills mode, the competition control will be checked
        if (compControl.isAutonomous() && compControl.isEnabled() && isField()){//return true if auton is on and the robot is enabled
            return true;
        }
        return false;//otherwise return false
    }
    if (mode == 2 && !isField()){//if in auton testing mode, always allow
        return true;
    }
    return false;//return false otherwise
}
bool confirmDriver(){//Confirms it is allowed to run driver control
    if (mode == 0 || mode == 1){//If in field control or skills mode, the competition control will be checked
        if (compControl.isDriverControl() && compControl.isEnabled() && isField()){//return true if driver is on and the robot is enabled
            return true;
        }
        return false;//otherwise return false
    }
    if (mode == 3 && !isField()){//if in driver mode, always allow
        return true;
    }
    return false;//return false otherwise
}
int selectAutonomous(){//method for selecting autons
    DisplaySelection selectAuton = DisplaySelection(4);//create display selection object
    strcpy(selectAuton.text[0], "Exit");//place names of autons in array
    strcpy(selectAuton.text[1], "Auton1");
    strcpy(selectAuton.text[2], "Auton2");
    strcpy(selectAuton.text[3], "Auton3");
    strcpy(selectAuton.text[4], "");
    return selectAuton.select();
}
void colorSelect(){//method for selecting field color
    DisplaySelection selectColor = DisplaySelection(2);//create display object
    strcpy(selectColor.text[0], "Red");//set array values to colors
    strcpy(selectColor.text[1], "Blue");
    strcpy(selectColor.text[2], "");
    strcpy(selectColor.text[3], "");
    strcpy(selectColor.text[4], "");
    colorRed = (selectColor.select() == 0);
}
int main() {
    robotMain.Screen.setFont(vex::fontType::mono40);
    robotMain.Screen.setPenColor("00FF00");
    ctrPrimary.Screen.clearScreen();
    ctrPrimary.Screen.setCursor(1,0);
    while(true){
        DisplaySelection selectMode = DisplaySelection(5); //Create Display object
        strcpy(selectMode.text[0], "Field Control");//set values in array to options
        strcpy(selectMode.text[1], "Skills Control");
        strcpy(selectMode.text[2], "Auton Testing");
        strcpy(selectMode.text[3], "Driver Control");
        strcpy(selectMode.text[4], "Exit");//for closing the program
        mode = selectMode.select();
        if(mode == 0){//Field control was selected
            calibrateGyros();//Calibrate gyro sensors
            colorSelect();//select team color
            int autonMode = selectAutonomous();//select auton to run
            while(true){//loop for competition
                if (!isField()){
                    ctrPrimary.Screen.clearScreen();
                    ctrPrimary.Screen.setCursor(1,0);
                    ctrPrimary.Screen.print("Connect to Field");
                    ctrPrimary.Screen.newLine();
                    ctrPrimary.Screen.print("(B) Close");
                    while(!ctrPrimary.ButtonB.pressing() && !isField()){wait(20);}
                    if(ctrPrimary.ButtonB.pressing()){
                        break;
                    }
                }
                while(!compControl.isEnabled()){//While disabled, user has option to close field control 
                    ctrPrimary.Screen.setCursor(1,0);
                    ctrPrimary.Screen.clearLine();
                    ctrPrimary.Screen.print("FC-Disabled");
                    while(!compControl.isEnabled()){wait(20);}
                }
                
                if(compControl.isEnabled() && compControl.isAutonomous()){//runs auton when enabled and autonomous
                        auton(autonMode);
                        while(compControl.isEnabled() && compControl.isAutonomous() && isField()){wait(20);}//Waits for auton to end (50 Hertz)
                }
                if(compControl.isEnabled() && compControl.isDriverControl()){//runs driver control when enabled and autonomous
                    driver();
                    while(compControl.isEnabled() && compControl.isDriverControl() && isField()){wait(20);}//Waits for driver control to end (50 Hertz)
                }
                stopAllMotors();
            }
            stopAllMotors();
        }
        if(mode == 1){//Skills mode- same as field control, except no color selection
            colorRed = true;
            int autonMode = selectAutonomous();
            ctrPrimary.Screen.clearScreen();
            while(true){
                if (!isField()){
                    ctrPrimary.Screen.clearScreen();
                    ctrPrimary.Screen.setCursor(1,0);
                    ctrPrimary.Screen.print("Connect to Field");
                    ctrPrimary.Screen.newLine();
                    ctrPrimary.Screen.print("(B) Close");
                    while(!ctrPrimary.ButtonB.pressing() && !isField()){wait(20);}
                    if(ctrPrimary.ButtonB.pressing()){
                        break;
                    }
                }
                while(!compControl.isEnabled()){//While disabled, user has option to close field control 
                    ctrPrimary.Screen.setCursor(1,0);
                    ctrPrimary.Screen.clearLine();
                    ctrPrimary.Screen.print("SK-Disabled");
                    while(!compControl.isEnabled()){wait(20);}
                }
                if(compControl.isEnabled() && compControl.isAutonomous()){
                        auton(autonMode);
                        while(compControl.isEnabled() && compControl.isAutonomous() && isField()){wait(20);}//Waits for auton to end (50 Hertz)
                }
                if(compControl.isEnabled() && compControl.isDriverControl()){
                    driver();
                    while(compControl.isEnabled() && compControl.isDriverControl() && isField()){wait(20);}//Waits for driver control to end (50 Hertz)
                }
                stopAllMotors();
            }
            stopAllMotors();
        }
        if(mode == 2){//Auton testing mode
            while (true){
                while(true){
                    ctrPrimary.Screen.clearScreen();
                    ctrPrimary.Screen.setCursor(1,0);
                    ctrPrimary.Screen.print("Setup Robot");
                    ctrPrimary.Screen.newLine();
                    ctrPrimary.Screen.print("(A) Done");//Waits until robot is placed properly for auton
                    if (ctrPrimary.ButtonA.pressing()){
                        break;
                    }
                    wait(50); //Update at 20 hertz
                }
                while(ctrPrimary.ButtonA.pressing()){wait(20);}
                calibrateGyros();
                int selection = selectAutonomous();
                if(selection == 0){
                    break;
                }
                colorSelect();//select color
                auton(selection);//run selected auton
                stopAllMotors();
                while(ctrPrimary.ButtonB.pressing()){wait(20);}//wait for exit button to be released
            }
        }
        if(mode == 3){
            calibrateGyros();
            driver();
            stopAllMotors();
            while(ctrPrimary.ButtonB.pressing()){wait(20);}//wait for exit button to be released
        }
        if(mode == 4){
            return 0;
        }
    }
}
