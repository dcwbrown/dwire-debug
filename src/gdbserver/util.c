u8 hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c-'0';
    }
    if (c >= 'a' && c <= 'f') {
        return c-'a'+10;
    }
    if (c >= 'A' && c <= 'F') {
        return c-'A'+10;
    }

    return 0;
}

u8 hex_to_byte(const char *cmd)
{
    return hex_nibble(cmd[1]) | (hex_nibble(cmd[0])<<4);
}
