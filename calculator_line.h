struct CalculatorLine {
    //NOTE: Could be a color swatch if out is empty
    float4 colorOut;
    char *in;
    char *out;
    char *units;
    int significantFigures;
};

struct CalculatorLines {
    int calculatorLineCount;
    int maxCalculatorLineCount;
    CalculatorLine *calculatorLines; //NOTE: Lives on per vm run arena
};
