struct UnitTestResults {
    GameState *gameState;
    int passedCount;
    int totalCount;
};

void runUnitTest(UnitTestResults *results,  char *code, double answer) {
    GameState *gameState = results->gameState;
    refreshVmMemoryArena();
    gameState->operations.clear();

    char *error = compileToByteCode(code, &gameState->operations, &gameState->calculatorLinesParent);
    VmMachineState machineState  = initVmMachineState();
    runCode(&machineState, gameState, &gameState->operations, true);

    double value = popAndGetValueNumber(&machineState, OP_CODE_FLOAT).as_float;

    results->totalCount++;
    float epsilon = 0.01;
    if((value - epsilon) < answer && (value + epsilon) > answer) {
        results->passedCount++;
    } else {
        printf("didn't pass: %s, got %f, wanted %f\n", code, value, answer);
    }
}

void runLanguageUnitTests(GameState *gameState) {
    UnitTestResults results = {};
    results.gameState = gameState;
    runUnitTest(&results, "3+3;", 6);
    runUnitTest(&results, "2~/3;", 1);
    runUnitTest(&results, "-3;", -3);
    runUnitTest(&results, "-3*-2;", 6);
    runUnitTest(&results, "-3-3*-2;", 3);
    runUnitTest(&results, "-3--2;", -1);
    runUnitTest(&results, "2^-2*2;", 0.5);

    // Basic arithmetic
    runUnitTest(&results, "1+1;", 2);
    runUnitTest(&results, "10-3;", 7);
    runUnitTest(&results, "4*5;", 20);
    runUnitTest(&results, "10/2;", 5);
    runUnitTest(&results, "7/2;", 3.5);

    // Unary negation
    runUnitTest(&results, "-1;", -1);
    runUnitTest(&results, "-0;", 0);
    runUnitTest(&results, "--3;", 3);
    runUnitTest(&results, "---3;", -3);

    // Addition & subtraction with negatives
    runUnitTest(&results, "-3+3;", 0);
    runUnitTest(&results, "3+-3;", 0);
    runUnitTest(&results, "-3+-3;", -6);
    runUnitTest(&results, "3--3;", 6);
    runUnitTest(&results, "-10+4;", -6);
    runUnitTest(&results, "0-5;", -5);

    // Multiplication with negatives
    runUnitTest(&results, "-2*3;", -6);
    runUnitTest(&results, "3*-2;", -6);
    runUnitTest(&results, "-3*-3;", 9);
    runUnitTest(&results, "-1*0;", 0);

    // Division with negatives
    runUnitTest(&results, "-6/2;", -3);
    runUnitTest(&results, "6/-2;", -3);
    runUnitTest(&results, "-6/-2;", 3);
    runUnitTest(&results, "-1/2;", -0.5);

    // Operator precedence: * and / before + and -
    runUnitTest(&results, "2+3*4;", 14);
    runUnitTest(&results, "3*4+2;", 14);
    runUnitTest(&results, "10-2*3;", 4);
    runUnitTest(&results, "2*3+4*5;", 26);
    runUnitTest(&results, "10/2+3;", 8);
    runUnitTest(&results, "3+10/2;", 8);
    runUnitTest(&results, "8/4+6/3;", 4);
    runUnitTest(&results, "1+2*3-4/2;", 5);

    // Exponentiation
    runUnitTest(&results, "2^3;", 8);
    runUnitTest(&results, "3^2;", 9);
    runUnitTest(&results, "10^0;", 1);
    runUnitTest(&results, "1^100;", 1);
    runUnitTest(&results, "0^5;", 0);
    runUnitTest(&results, "4^0.5;", 2);
    runUnitTest(&results, "2^10;", 1024);
    runUnitTest(&results, "-2^3;", -8);
    runUnitTest(&results, "2^-1;", 0.5);
    runUnitTest(&results, "2^-2;", 0.25);
    runUnitTest(&results, "2^-3;", 0.125);

    // Exponentiation precedence over * and /
    runUnitTest(&results, "2^2*3;", 12);
    runUnitTest(&results, "3*2^2;", 12);
    runUnitTest(&results, "2^3/4;", 2);
    runUnitTest(&results, "2^2+3;", 7);
    runUnitTest(&results, "3+2^2;", 7);
    runUnitTest(&results, "2^2*2^3;", 32);
    runUnitTest(&results, "2^-2*2;", 0.5);
    runUnitTest(&results, "2^-2*8;", 2);
    runUnitTest(&results, "4*2^-1;", 2);

    // Mixed precedence chains
    runUnitTest(&results, "(-2)^2;", 4);   // this breaks the ast tree
    runUnitTest(&results, "-2^2;", -4);    // unary minus applied after exponent
    runUnitTest(&results, "10-2^2*2;", 2);
    runUnitTest(&results, "-2^2+3*2;", 2);
    runUnitTest(&results, "2+3*4-1;", 13);
    runUnitTest(&results, "2^3+4*2-1;", 15);
    runUnitTest(&results, "3*2^2-4/2;", 10);
    runUnitTest(&results, "-3+2*4;", 5);
    runUnitTest(&results, "5*-2+3^2;", -1);
    runUnitTest(&results, "1+2+3+4+5;", 15);
    runUnitTest(&results, "2*3*4;", 24);
    runUnitTest(&results, "100/10/2;", 5);   // left-to-right division

    // Zero and identity
    runUnitTest(&results, "0+0;", 0);
    runUnitTest(&results, "0*100;", 0);
    runUnitTest(&results, "0/1;", 0);
    runUnitTest(&results, "1*1;", 1);
    runUnitTest(&results, "5+0;", 5);
    runUnitTest(&results, "5-0;", 5);
    runUnitTest(&results, "5*1;", 5);
    runUnitTest(&results, "5/1;", 5);

    // Decimals
    runUnitTest(&results, "0.5+0.5;", 1);
    runUnitTest(&results, "1.5*2;", 3);
    runUnitTest(&results, "2.5-1.5;", 1);
    runUnitTest(&results, "0.5+0.5;", 1.0);
    runUnitTest(&results, "0.25*4;", 1.0);
    runUnitTest(&results, "1.5+1.5;", 3.0);
    runUnitTest(&results, "1.5^2;", 2.25);

    // Basic parentheses
    runUnitTest(&results, "(2+3);", 5);
    runUnitTest(&results, "(2*3);", 6);
    runUnitTest(&results, "(10-3);", 7);
    runUnitTest(&results, "(10/2);", 5);

    // Exponent right hand associatavity precendance
    runUnitTest(&results, "2^3^2;", 512);

    // Parentheses overriding precedence
    runUnitTest(&results, "(2+3)*4;", 20);
    runUnitTest(&results, "4*(2+3);", 20);
    runUnitTest(&results, "(10-3)*2;", 14);
    runUnitTest(&results, "2*(10-3);", 14);
    runUnitTest(&results, "(2+3)*(4+5);", 45);
    runUnitTest(&results, "(10-2)*(3+1);", 32);
    runUnitTest(&results, "(6+4)/2;", 5);
    runUnitTest(&results, "10/(2+3);", 2);
    runUnitTest(&results, "(4+6)/(2+3);", 2);

    // Nested parentheses
    runUnitTest(&results, "((2+3));", 5);
    runUnitTest(&results, "((2+3)*2);", 10);
    runUnitTest(&results, "(2*(3+4));", 14);
    runUnitTest(&results, "((2+3)*(4+5));", 45);
    runUnitTest(&results, "(((3)));", 3);
    runUnitTest(&results, "((2+3)*2)+1;", 11);
    runUnitTest(&results, "2*((3+4)*2);", 28);
    runUnitTest(&results, "((2+3)*(3+2))*2;", 50);
    runUnitTest(&results, "(2+(3*(4+1)));", 17);
    runUnitTest(&results, "((2+3)+(4*2));", 13);

    // Parentheses with negatives
    runUnitTest(&results, "(-3);", -3);
    runUnitTest(&results, "(-3+5);", 2);
    runUnitTest(&results, "-(3+2);", -5);
    runUnitTest(&results, "-(3*2);", -6);
    runUnitTest(&results, "-(-3);", 3);
    runUnitTest(&results, "-(-3+1);", 2);
    runUnitTest(&results, "(-2)*(-3);", 6);
    runUnitTest(&results, "(-2+5)*(-1+4);", 9);
    runUnitTest(&results, "-(2+3)*2;", -10);
    runUnitTest(&results, "2*-(3+1);", -8);

    // Parentheses with exponents
    runUnitTest(&results, "(2+2)^2;", 16);
    runUnitTest(&results, "2^(1+1);", 4);
    runUnitTest(&results, "(2+1)^3;", 27);
    runUnitTest(&results, "(2^2)^2;", 16);
    runUnitTest(&results, "2^(2+1)*3;", 24);
    runUnitTest(&results, "(3-1)^(1+2);", 8);
    runUnitTest(&results, "(-2)^2;", 4);
    runUnitTest(&results, "(-2)^3;", -8);
    runUnitTest(&results, "(-3)^2;", 9);
    runUnitTest(&results, "2^(-1+3);", 4);

    // Parentheses in longer expressions
    runUnitTest(&results, "1+(2*3)+4;", 11);
    runUnitTest(&results, "(1+2)*(3+4)+5;", 26);
    runUnitTest(&results, "10-(2+3)*2;", 0);
    runUnitTest(&results, "(10-2)*3+4;", 28);
    runUnitTest(&results, "2*(3+(4*5));", 46);
    runUnitTest(&results, "(2+3)*2-(4+1);", 5);
    runUnitTest(&results, "((4+6)*2)/4;", 5);
    runUnitTest(&results, "3*(2+(8/4));", 12);
    runUnitTest(&results, "(3^2+1)*2;", 20);
    runUnitTest(&results, "2*(3^(1+1));", 18);

    // sqrt()
    runUnitTest(&results, "sqrt(4);", 2);
    runUnitTest(&results, "sqrt(9);", 3);
    runUnitTest(&results, "sqrt(16);", 4);
    runUnitTest(&results, "sqrt(25);", 5);
    runUnitTest(&results, "sqrt(100);", 10);
    runUnitTest(&results, "sqrt(0);", 0);
    runUnitTest(&results, "sqrt(1);", 1);
    runUnitTest(&results, "sqrt(2);", 1.41421356);
    runUnitTest(&results, "sqrt(0.25);", 0.5);

    // sqr()
    runUnitTest(&results, "sqr(2);", 4);
    runUnitTest(&results, "sqr(3);", 9);
    runUnitTest(&results, "sqr(4);", 16);
    runUnitTest(&results, "sqr(10);", 100);
    runUnitTest(&results, "sqr(0);", 0);
    runUnitTest(&results, "sqr(1);", 1);
    runUnitTest(&results, "sqr(-2);", 4);
    runUnitTest(&results, "sqr(-5);", 25);
    runUnitTest(&results, "sqr(0.5);", 0.25);

    // sin() — radians
    runUnitTest(&results, "sin(0);", 0);
    runUnitTest(&results, "sin(3.14159265/2);", 1);       // sin(π/2) = 1
    runUnitTest(&results, "sin(3.14159265);", 0);          // sin(π)   = 0
    runUnitTest(&results, "sin(-3.14159265/2);", -1);      // sin(-π/2) = -1

    // cos() — radians
    runUnitTest(&results, "cos(0);", 1);
    runUnitTest(&results, "cos(3.14159265);", -1);         // cos(π)   = -1
    runUnitTest(&results, "cos(3.14159265/2);", 0);        // cos(π/2) = 0
    runUnitTest(&results, "cos(-3.14159265);", -1);

    // tan() — radians
    runUnitTest(&results, "tan(0);", 0);
    runUnitTest(&results, "tan(3.14159265);", 0);          // tan(π)   = 0
    runUnitTest(&results, "tan(3.14159265/4);", 1);        // tan(π/4) = 1
    runUnitTest(&results, "tan(-3.14159265/4);", -1);

    // asin() — returns radians
    runUnitTest(&results, "asin(0);", 0);
    runUnitTest(&results, "asin(1);", 1.57079633);         // π/2
    runUnitTest(&results, "asin(-1);", -1.57079633);       // -π/2
    runUnitTest(&results, "asin(0.5);", 0.52359878);       // π/6

    // acos() — returns radians
    runUnitTest(&results, "acos(1);", 0);
    runUnitTest(&results, "acos(0);", 1.57079633);         // π/2
    runUnitTest(&results, "acos(-1);", 3.14159265);        // π
    runUnitTest(&results, "acos(0.5);", 1.04719755);       // π/3

    // atan() — returns radians
    runUnitTest(&results, "atan(0);", 0);
    runUnitTest(&results, "atan(1);", 0.78539816);         // π/4
    runUnitTest(&results, "atan(-1);", -0.78539816);       // -π/4
    runUnitTest(&results, "atan(0);", 0);

    // Functions combined with operators
    runUnitTest(&results, "sqrt(4)+1;", 3);
    runUnitTest(&results, "sqrt(4)*3;", 6);
    runUnitTest(&results, "sqr(3)+1;", 10);
    runUnitTest(&results, "sqr(2)*sqr(3);", 36);
    runUnitTest(&results, "sqrt(sqr(5));", 5);
    runUnitTest(&results, "sqr(sqrt(9));", 9);
    runUnitTest(&results, "sqrt(4+5);", 3);
    runUnitTest(&results, "sqrt(3*3);", 3);
    runUnitTest(&results, "sqr(2+1);", 9);
    runUnitTest(&results, "sqr(10-7);", 9);
    runUnitTest(&results, "2*sqrt(9)+1;", 7);
    runUnitTest(&results, "sqrt(16)/2;", 2);
    runUnitTest(&results, "sqr(2)^2;", 16);         // sqr(2)=4, 4^2=16
    runUnitTest(&results, "sqrt(2^4);", 4);          // 2^4=16, sqrt=4

    // Trig combined with operators
    runUnitTest(&results, "sin(0)+1;", 1);
    runUnitTest(&results, "cos(0)*5;", 5);
    runUnitTest(&results, "sin(0)+cos(0);", 1);
    runUnitTest(&results, "sqr(sin(0))+sqr(cos(0));", 1);   // Pythagorean identity
    runUnitTest(&results, "2*cos(0)+1;", 3);
    runUnitTest(&results, "atan(1)*4;", 3.14159265);         // π

    // Nested functions
    runUnitTest(&results, "sqrt(sqrt(16));", 2);
    runUnitTest(&results, "sqr(sqr(2));", 16);
    runUnitTest(&results, "sin(asin(1));", 1);
    runUnitTest(&results, "cos(acos(1));", 1);
    runUnitTest(&results, "tan(atan(1));", 1);
    runUnitTest(&results, "asin(sin(0));", 0);
    runUnitTest(&results, "acos(cos(0));", 0);
    runUnitTest(&results, "atan(tan(0));", 0);


    // sqrt() with order of operations inside
    runUnitTest(&results, "sqrt(2+2);", 2);
    runUnitTest(&results, "sqrt(3*3);", 3);
    runUnitTest(&results, "sqrt(2^4);", 4);
    runUnitTest(&results, "sqrt(10-6);", 2);
    runUnitTest(&results, "sqrt(100/4);", 5);
    runUnitTest(&results, "sqrt(2*8);", 4);
    runUnitTest(&results, "sqrt(1+3*5);", 4);           // 1+15=16
    runUnitTest(&results, "sqrt(2^2+2^2);", 2.82842712); // sqrt(8)
    runUnitTest(&results, "sqrt(3^2+4^2);", 5);          // Pythagorean: 9+16=25
    runUnitTest(&results, "sqrt(5^2-3^2);", 4);          // 25-9=16
    runUnitTest(&results, "sqrt(10*10-6*8);", 7.211103);         // 100-48=52... wait, sqrt(52)
    runUnitTest(&results, "sqrt((2+3)*5);", 5);           // 5*5=25
    runUnitTest(&results, "sqrt((3+1)^2);", 4);
    runUnitTest(&results, "sqrt(9*4-2^4);", 4.472135955);
    runUnitTest(&results, "sqrt(-1*-9);", 3);             // -1*-9=9

    // sqr() with order of operations inside
    runUnitTest(&results, "sqr(1+1);", 4);
    runUnitTest(&results, "sqr(2*3);", 36);
    runUnitTest(&results, "sqr(10-8);", 4);
    runUnitTest(&results, "sqr(10/2);", 25);
    runUnitTest(&results, "sqr(2^3);", 64);
    runUnitTest(&results, "sqr(1+2+3);", 36);
    runUnitTest(&results, "sqr(2*3-4);", 4);             // 6-4=2, sqr=4
    runUnitTest(&results, "sqr(3*3-5);", 16);            // 9-5=4
    runUnitTest(&results, "sqr(2+3*2);", 64);            // 2+6=8
    runUnitTest(&results, "sqr((2+3)*2);", 100);         // 5*2=10
    runUnitTest(&results, "sqr(4^2-2^3);", 64);          // 16-8=8
    runUnitTest(&results, "sqr(-3*-2);", 36);            // -3*-2=6
    runUnitTest(&results, "sqr(-2+4);", 4);              // -2+4=2

    // sin() with order of operations inside
    runUnitTest(&results, "sin(3.14159265/2);", 1);
    runUnitTest(&results, "sin(3.14159265/6*1);", 0.5);      // π/6 = 0.5
    runUnitTest(&results, "sin(3.14159265*2);", 0);           // sin(2π) = 0
    runUnitTest(&results, "sin(3.14159265/4*2);", 1);         // sin(π/2)
    runUnitTest(&results, "sin(1+1-2);", 0);                  // sin(0)
    runUnitTest(&results, "sin(2^2-4);", 0);                  // sin(0)
    runUnitTest(&results, "sin(3.14159265*(1+1));", 0);       // sin(2π)

    // cos() with order of operations inside
    runUnitTest(&results, "cos(3.14159265*1);", -1);
    runUnitTest(&results, "cos(3.14159265/3*2);", -0.5);      // cos(2π/3)
    runUnitTest(&results, "cos(3.14159265-3.14159265);", 1);  // cos(0)
    runUnitTest(&results, "cos(2^2-4);", 1);                  // cos(0)
    runUnitTest(&results, "cos(1*0);", 1);                    // cos(0)

    // tan() with order of operations inside
    runUnitTest(&results, "tan(3.14159265/4*1);", 1);
    runUnitTest(&results, "tan(3.14159265*2-3.14159265*2);", 0); // tan(0)
    runUnitTest(&results, "tan(2-2);", 0);                       // tan(0)
    runUnitTest(&results, "tan(3.14159265/2-3.14159265/4);", 1); // tan(π/4)

    // asin/acos/atan with order of operations inside
    runUnitTest(&results, "asin(1/2);", 0.52359878);    // asin(0.5) = π/6
    runUnitTest(&results, "asin(2-1);", 1.57079633);    // asin(1)
    runUnitTest(&results, "asin(-1*1);", -1.57079633);  // asin(-1)
    runUnitTest(&results, "acos(2-2);", 1.57079633);    // acos(0)
    runUnitTest(&results, "acos(2/2);", 0);             // acos(1)
    runUnitTest(&results, "acos(-1*1);", 3.14159265);   // acos(-1) = π
    runUnitTest(&results, "atan(4/4);", 0.78539816);    // atan(1) = π/4
    runUnitTest(&results, "atan(2-2);", 0);             // atan(0)
    runUnitTest(&results, "atan(2*2-3);", 0.78539816);  // atan(1)

    // Functions with expressions inside AND outside
    runUnitTest(&results, "sqrt(3^2+4^2)*2;", 10);      // sqrt(25)*2
    runUnitTest(&results, "2+sqrt(2+2)*3;", 8);          // 2+2*3
    runUnitTest(&results, "sqr(2+1)*2+1;", 19);          // sqr(3)=9, 9*2+1
    runUnitTest(&results, "sqr(2*3)/sqr(3);", 4);        // 36/9
    runUnitTest(&results, "sqrt(4*9)/sqrt(4);", 3);      // 6/2
    runUnitTest(&results, "sqr(1+1)+sqr(2+1);", 13);     // 4+9
    runUnitTest(&results, "sqrt(4)+sqrt(9)+sqrt(16);", 9);
    runUnitTest(&results, "sin(0)*sqr(5)+cos(0);", 1);   // 0+1
    runUnitTest(&results, "sqr(cos(0)+1);", 4);          // sqr(2)
    runUnitTest(&results, "sqrt(sqr(2+1));", 3);         // sqrt(9)
    runUnitTest(&results, "sqr(sqrt(2^4+2^4));", 32);    // sqrt(32) then sqr
    // sin( (5 + 2) * -sqrt(3^2))


    printf("Passed %d out of %d tests.\n", results.passedCount, results.totalCount);
}