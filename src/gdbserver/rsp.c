ssize_t read_command(int fd, char *buf, size_t size)
{
    ssize_t r;
    size_t i = 0;
    char c;

    while (1) {
        r = Read((FileHandle)fd, &c, 1);
        if (r < 1) {
            return r;
        }
        if (c == '$') {
            break;
        }
    }

    while (1) {
        r = Read((FileHandle)fd, &c, 1);
        if (r < 1) {
            return r;
        }
        if (c == '#') {
            break;
        }
        buf[i++] = c;
    }

    // TODO: check crc
    r = Read((FileHandle)fd, &c, 1);
    r = Read((FileHandle)fd, &c, 1);
    buf[i] = '\0';

    // send ack
    Write((FileHandle)fd, "+", 1);

    return i;
}

static size_t write_all(int fd, const char *buf, size_t len)
{
    size_t off = 0;
    size_t r;

    while (off < len) {
        r = Write((FileHandle)fd, buf+off, len-off);
        if (r < 1) {
            return r;
        }
        off += r;
    }

    return off;
}

static size_t write_resp(int fd, const char *buf)
{
    size_t len = strlen(buf);
    size_t r = 0;
    char tmp[3];
    uint8_t chsum = 0;

    for (int i = 0; i < len; i++) {
        chsum += buf[i];
    }

    r += write_all(fd, "$", 1);
    r += write_all(fd, buf, len);
    r += write_all(fd, "#", 1);

    snprintf(tmp, sizeof(tmp), "%02x", chsum);

    r += write_all(fd, tmp, 2);

    return r;
}

void send_message(int fd, const char *txt)
{
    size_t len = strlen(txt);
    char buf[2*len+2], *p;
    uint_fast8_t i;

    buf[0] = 'O';
    p = buf+1;

    for (i = 0; i < len; i++) {
        p[i*2] = (int)((txt[i] & 0xf0) >> 4) + '0';
        if (p[i*2] > '9')
            p[i*2] += 7;
        p[i*2+1] = (int)(txt[i] & 0xf) + '0';
        if (p[i*2+1] > '9')
            p[i*2+1] += 7;
    }

    p[i*2] = '\0';

    write_resp(fd, buf);
}

void cmd_read_registers(int fd)
{
    Registers regs;
    char      buf[countof(regs)*2 + 1];

    target_read_registers(regs);

    for (int i=0; i<countof(regs); i++) {
        buf[2*i]   = HexChar(regs[i] / 16);
        buf[2*i+1] = HexChar(regs[i] % 16);
    }
    buf[countof(regs)*2] = 0;

    write_resp(fd, buf);
}

void cmd_memory_read(int fd, const char *cmd)
{
    int addr, len;
    uint_fast8_t i;

    sscanf(cmd, "%x,%x", &addr, &len);

    uint8_t buf[len];
    char out[len*2+1];

    target_read_addr(addr, buf, len);

    for (i = 0; i < len; i++) {
        out[i*2]   = HexChar(buf[i] / 16);
        out[i*2+1] = HexChar(buf[i] % 16);
    }
    out[2*i] = '\0';

    write_resp(fd, out);
}

void cmd_memory_write(int fd, const char *cmd)
{
    int addr, len;
    char *b;

    sscanf(cmd, "%x,%x", &addr, &len);

    b = strchr(cmd, ':') + 1;

    size_t l = strlen(b);
    uint8_t buf[l/2];

    for (int i = 0; i < l; i+=2) {
        buf[i/2] = hex_to_byte(b+i);
    }

    target_write_addr(addr, buf, sizeof(buf));

    write_resp(fd, "OK");
}


void cmd_write_registers(int fd, const char *cmd)
{
    size_t len = strlen(cmd);
    uint8_t buf[len/2];

    for (int i = 0; i < len; i+=2) {
        buf[i/2] = hex_to_byte(cmd+i);
    }

    target_write_registers(buf, len/2);

    write_resp(fd, "OK");
}

void cmd_set_breakpoint(int fd, const char *cmd)
{
    int addr;

    sscanf(cmd+1, "%x", &addr);

    target_set_breakpoint(addr);

    write_resp(fd, "OK");
}

void cmd_clear_breakpoint(int fd, const char *cmd)
{
    target_clear_breakpoint();

    write_resp(fd, "OK");
}

void handle_command(int fd, const char *cmd)
{
    switch (cmd[0]) {
        case '?':
            write_resp(fd, "S05");
            break;
        case 'c':
            target_continue(fd);
            write_resp(fd, "S05");
            break;
        case 's':
            target_step();
            write_resp(fd, "S05");
            break;
        case 'g':
            cmd_read_registers(fd);
            break;
        case 'G':
            cmd_write_registers(fd, cmd+1);
            break;
        case 'm':
            cmd_memory_read(fd, cmd+1);
            break;
        case 'M':
            cmd_memory_write(fd, cmd+1);
            break;
        case 'Z':
            if (cmd[1] == '1') {
                cmd_set_breakpoint(fd, cmd+2);
                break;
            }
        case 'z':
            if (cmd[1] == '1') {
                cmd_clear_breakpoint(fd, cmd+2);
                break;
            }
        default:
            write_resp(fd, "");
            break;
    }
}
