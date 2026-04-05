#define DEBUG_PRINT_AST_TREE 1

void printAstNodeAdvanced(AstNode *node, char *prefix, bool isLast) {
    if (!node) return;

    // 1. Print the current indentation and the branch character
    printf("%s", prefix);
    printf(isLast ? "└── " : "├── ");

    // 2. Print the token
    if(node->type == AST_TYPE_PARENT) {
        printf("PARENT NODE: ");
        DEBUG_lexPrintToken(&node->token);
    } else {
        DEBUG_lexPrintToken(&node->token);
    }

    printf("\n");

    // 3. Prepare the prefix for the children
    // We append either a vertical bar or a space depending on if this node has more siblings
    char newPrefix[256];
    snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "│   ");

    // 4. Recurse to the first child
    if (node->child) {
        AstNode *c = node->child;
        while (c != NULL) {
            // A child is the "last" if its next pointer is null
            printAstNodeAdvanced(c, newPrefix, c->next == NULL);
            c = c->next;
        }
    }
}

void printAstTree(AstTree *tree) {
#if DEBUG_PRINT_AST_TREE
    if (!tree || !tree->start) {
        printf("(Empty Tree)\n");
        return;
    }

    AstNode *node = tree->start;
    while(node) {
        // Start recursion with an empty prefix and true (since root is the only node at its level)
        printAstNodeAdvanced(node, "", true);
        node = node->next;
    }
#endif

}