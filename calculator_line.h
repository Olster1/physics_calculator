struct CalculatorLine {
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
