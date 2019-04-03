char decodeKeyPress(unsigned char scanCode){
    switch (scanCode) {
        case 0x1E: //A pressed
            return 'A';
        case 0x1F: //S pressed
            return 'S';
        case 0x20: //D pressed
            return 'D';
        case 0x21: //F pressed
            return 'F';
        default:
            return ' ';
    }
}
