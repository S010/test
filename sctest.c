#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include <windows.h>
#include <winscard.h>

#define NELEMS(a) (sizeof(a)/sizeof(a[0]))

#define DEFAULT_DISCONNECT_ACTION SCARD_UNPOWER_CARD
#define INDENT "    "

/* 
 * Some definitions:
 *  ICC    -- integrated circuit card
 *  TAL    -- terminal application layer
 *  TTL    -- terminal transport layer
 *  T0     -- serial protocol 
 *  T1     -- block protocol
 *  C-APDU -- command application protocol data unit
 *  R-APDU -- response application protocol data unit
 */

typedef unsigned char byte;

struct opts {
    DWORD scope;
    int mode;
    int reader;
} opts;

enum e_modes {
    NORMAL,
    LIST_READERS
};



struct ttl_t0_du {
    byte cla;
    byte ins;
    byte p1;
    byte p2;
    byte p3;
    byte data[256];
};

struct apdu_body {
    int len; /* Lc, -1 means data is not present */
    byte data[256];
    int max_r_len; /* Le, -1 means not present */
};

struct c_apdu {
    byte cla; /* instruction class */
    byte ins; /* instruction code */
    byte p1;  /* p1,p2 are reference bytes, complementing ins */
    byte p2;
    struct apdu_body body;
};

struct r_apdu {
    byte sw1;
    byte sw2;
    struct apdu_body body;
};

void
print_bin_string(const char *s, size_t len) {
    while (len--) {
        if (isgraph(*s))
            putchar(*s);
        else
            printf("\\x%02x", *s);
        ++s;
    }
}

void
print_r_apdu(const char *name, struct r_apdu *r) {
    printf("%s {\n", name ? name : "r_apdu");
    printf("%sSW1 = 0x%x\n", INDENT, r->sw1);
    printf("%sSW2 = 0x%x\n", INDENT, r->sw2);
    printf("%sbody = ", INDENT);
    if (r->body.len == -1)
        printf("(not present)\n");
    else
        print_bin_string(r->body.data, r->body.len);
    printf("}\n");
}

int
ttl_io(SCARDHANDLE card, DWORD protocol, struct c_apdu *c_apdu, struct r_apdu *r_apdu) {
    LONG rc;
    SCARD_IO_REQUEST send_io_req, recv_io_req;
    BYTE send_buf[256 + 5], recv_buf[NELEMS(send_buf)];
    DWORD send_buf_len, recv_buf_len;
    
    if (protocol != SCARD_PROTOCOL_T0) {
        fprintf(stderr, "only T0 protocol is currently supported");
        return -1;
    }
    
    memset(&send_io_req, 0, sizeof(send_io_req));
    memset(&recv_io_req, 0, sizeof(recv_io_req));
    memset(send_buf, 0, sizeof(send_buf));
    memset(recv_buf, 0, sizeof(recv_buf));
    send_buf_len = recv_buf_len = 0;
    
    if (c_apdu->body.len == -1) {
        memcpy(send_buf, c_apdu, 4);
        send_buf_len = 4;
    } else {
        memcpy(send_buf, c_apdu, 4);
        send_buf[4] = (BYTE) c_apdu->body.len;
        memcpy(send_buf + 5, c_apdu->body.data, c_apdu->body.len);
        send_buf_len = 4 + 1 + c_apdu->body.len;
    }
    
    recv_buf_len = 2;
    if (c_apdu->body.max_r_len > 0)
        recv_buf_len += c_apdu->body.max_r_len;
    
    fprintf(stderr, "calling SCardTransmit\n");
    if ((rc = SCardTransmit(card, protocol == SCARD_PROTOCOL_T0 ? SCARD_PCI_T0 : SCARD_PCI_T1, send_buf, send_buf_len, NULL, recv_buf, &recv_buf_len)) != SCARD_S_SUCCESS) {
        fprintf(stderr, "SCardTransmit failed with code %d\n", rc);
        return -1;
    }
    
    memcpy(r_apdu, recv_buf + recv_buf_len - 2, 2);
    if (recv_buf_len > 2) {
        memcpy(r_apdu->body.data, recv_buf, recv_buf_len - 2);
        r_apdu->body.len = recv_buf_len - 2;

    } else {
        r_apdu->body.len = -1;
    }
    
    return 0;  
}

void
usage(void) {
    printf(
        "usage: sctest -l -s U|S -c <n>\n"
        "    -s -- scope, user or system\n"
        "    -l -- list card readers\n"
        "    -c <n> -- use card reader <n>\n"
    );
    exit(0);
} 

void
proc_args(int argc, char **argv) {
    int i;

    opts.scope = SCARD_SCOPE_USER;

    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-s")) {
            if (++i >= argc)
                usage();
            if (*argv[i] == 'S')
                opts.scope = SCARD_SCOPE_SYSTEM;
            else
                opts.scope = SCARD_SCOPE_USER;
        } else if (!strcmp(argv[i], "-l")) {
            opts.mode = LIST_READERS;
        } else if (!strcmp(argv[i], "-c")) {
            if (++i >= argc)
                usage();
            opts.reader = atoi(argv[i]);
        } else {
            usage();
        }
    }
} 

void
print_indent(int n) {
    while (n--)
        printf("%s", INDENT);
}

int
fail(int code, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);

    return code;
} 

int
get_card_readers(SCARDCONTEXT ctx, const char **readers) {
    char *buf;
    size_t n = SCARD_AUTOALLOCATE;
    int rc;
    
    rc = SCardListReaders(ctx, NULL, (char *) &buf, &n);
    if (readers)
        *readers = buf;
    return rc;
}

char *
get_multistring(const char *ms, int n) {
    int i;
    
    for (i = 0; i != n; ++i) {
        ms += strlen(ms);
        if (*(++ms) == '\0')
            return NULL;
    }
    return (char *) ms;
}

int
count_multistring(const char *ms) {
    int n;
    
    for (n = 0; ;++ms) {
        if (*ms == '\0') {
            ++n;
            if (*(++ms) == '\0')
                break;
        }
    }
    return n;
}

void
print_readers(const char *ms) {
    int i;
    size_t len;
    
    printf("card readers present in the system:\n");
    for (i = 0; ; ++i) {
        printf("    %d \"%s\"\n", i, ms);
        ms += strlen(ms);
        if (*(++ms) == '\0')
            return;
    }
}

struct card_state {
    DWORD state;
    DWORD proto;
    unsigned char *atr;
    size_t atr_len;
};

int
get_card_state(SCARDHANDLE card, struct card_state *cst) {
    char *reader_names = NULL;
    size_t reader_names_size = 0;
    int rc;
    
    cst->atr = NULL;
    cst->atr_len = SCARD_AUTOALLOCATE;
    return SCardStatus(card, NULL, &reader_names_size, &cst->state, &cst->proto, (char *) &cst->atr, &cst->atr_len);
}

const char *
card_state_to_str(DWORD st) {
    switch (st) {
    case SCARD_ABSENT:
        return "ABSENT";
    case SCARD_PRESENT:
        return "PRESENT";
    case SCARD_SWALLOWED:
        return "SWALLOWED";
    case SCARD_POWERED:
        return "POWERED";
    case SCARD_NEGOTIABLE:
        return "NEGOTIABLE";
    case SCARD_SPECIFIC:
        return "SPECIFIC";
    default:
        return "UNKNOWN_STATE";
    }
}

const char *
card_proto_to_str(DWORD st) {
    switch (st) {
    case SCARD_PROTOCOL_RAW:
        return "RAW";
    case SCARD_PROTOCOL_T0:
        return "T0";
    case SCARD_PROTOCOL_T1:
        return "T1";
    default:
        return "UNKNOWN_PROTO";
    }
}

void
print_atr_bytes(int indent, struct card_state *cst) {
    size_t i;
    const char *t0_byte_names[] = { "TS", "T0", "TB1", "TC1", NULL };
    const char *t1_byte_names[] = { "TS", "T0", "TB1", "TC1", "TD1", "TD2", "TA3", "TB3", "TCK", NULL };
    const char **byte_names = NULL;
    const char *atr = cst->atr;
    size_t atr_len = cst->atr_len;
#   define BYTE_NAME_FMT "%3s"

    switch(cst->proto) {
    case SCARD_PROTOCOL_T0:
        byte_names = t0_byte_names;
        break;
    case SCARD_PROTOCOL_T1:
        byte_names = t1_byte_names;
        break;
    default:
        return;
    }
    
    for (i = 0; i < atr_len && byte_names[i] != NULL; ++i) {
        print_indent(indent);
        printf(BYTE_NAME_FMT " = 0x%02x\n", byte_names[i], (unsigned char) atr[i]);
    }
}



void
print_atr_str(int indent, struct card_state *cst) {
    size_t i;
    const char *atr = cst->atr;
    size_t atr_len = cst->atr_len;
    
    print_indent(indent);
    printf("\"");
    for (i = 0; i < atr_len; ++i)
        if (!isgraph(atr[i]))
            printf("\\x%02x", (unsigned char) atr[i]);
        else
            putchar(atr[i]);
    printf("\"");
}

void
print_card_state(struct card_state *cst) {
    size_t i;
    const char *state_str, *proto_str;
    const char *p;
    
    state_str = card_state_to_str(cst->state);
    proto_str = card_proto_to_str(cst->proto);
    
    printf("card state:\n"
           INDENT "state = %s\n"
           INDENT "proto = %s\n"
           INDENT "atr   = ",
           state_str, proto_str);
    print_atr_str(0, cst);
    putchar('\n');
    print_atr_bytes(2, cst);
}

/* check if atr string conforms to emv standard */
int
is_emv_atr(int indent, struct card_state *cst) {
    int is_okay;
    unsigned char *atr = cst->atr;
    const size_t atr_len = cst->atr_len;
    
    is_okay = 0;
    
    if (cst->proto == SCARD_PROTOCOL_T0) {
        if (atr_len < 4) {
            print_indent(indent);
            printf("atr string is too short\n");
        } else if (atr[0] != 0x3f && atr[0] != 0x3b) {
            print_indent(indent);
            printf("TS must be 0x3f or 0x3b\n");
        } else if ((atr[1] & 0x60) != 0x60) {
            print_indent(indent);
            printf("T0 must be 0x6?\n");
        } else if (atr[2] != 0x0) {
            print_indent(indent);
            printf("TB1 must be 0\n");
        } else {
            is_okay = 1;
        }
    } else if (cst->proto == SCARD_PROTOCOL_T1) {
        is_okay = 1;
    } else {
        printf("unknown protocol\n");
    }
    
    return is_okay;
}

int
sctest(void) {
    SCARDCONTEXT ctx;
    SCARDHANDLE card;
    int rc;
    char *readers = NULL;
    char *reader_dev_name = NULL;
    struct card_state card_state;
    enum conventions {
        DIRECT,
        INVERSE
    } convention = DIRECT;
    struct c_apdu c_apdu;
    struct r_apdu r_apdu;

    printf("creating context... ");
    
    if (SCardEstablishContext(opts.scope, NULL, NULL, &ctx) != SCARD_S_SUCCESS)
        return fail(1, "failed\n");
    else
        printf("ok\n");
    
    if ((rc = get_card_readers(ctx, &readers)) != SCARD_S_SUCCESS) {
        if (rc == SCARD_E_NO_READERS_AVAILABLE)
            return fail(-1, "no card readers present in the system\n");
        else
            return fail(-1, "SCardListReaders error %d", rc);
    }
    
    switch (opts.mode) {
    case LIST_READERS:
        print_readers(readers);
        break;
    case NORMAL:
    default:
        if ((reader_dev_name = get_multistring(readers, opts.reader)) == NULL) {
            SCardReleaseContext(ctx);
            return fail(-1, "no such card reader\n");
        } else
            printf("using card reader \"%s\"\n", reader_dev_name);
        
        printf("connecting to card reader... ");
        if ((rc = SCardConnect(ctx, reader_dev_name, SCARD_SHARE_DIRECT, 0, &card, SCARD_PROTOCOL_UNDEFINED)) != SCARD_S_SUCCESS) {
            SCardReleaseContext(ctx);
            switch (rc) {
            case SCARD_E_NO_SMARTCARD:
                return fail(-1, "no smart card in device\n");
                break;
            default:
                return fail(-1, "SCardConnect: error %d", rc);
                break;
            }
        } else
            printf("ok\n");
        
        if ((rc = get_card_state(card, &card_state)) != SCARD_S_SUCCESS) {
            SCardDisconnect(card, DEFAULT_DISCONNECT_ACTION);
            SCardReleaseContext(ctx);
            switch (rc) {
            default:
                return fail(-1, "SCardStatus: error %d\n");
            }
        }
        print_card_state(&card_state);
        
        printf("checking atr for conformance to emv... ");
        if (is_emv_atr(1, &card_state))
            printf("ok\n");
        
#       define FILE_STR "1PAY.SYS.DDF01"
        /* TODO */
        if (card_state.proto == SCARD_PROTOCOL_T0) {
            c_apdu.cla = 0x0;
            c_apdu.ins = 0xb2;
            c_apdu.p1 = 0x4;
            c_apdu.p2 = 0x0;
            c_apdu.body.len = strlen(FILE_STR);
            strcpy(c_apdu.body.data, FILE_STR);
            c_apdu.body.max_r_len = 0;
            
            if (ttl_io(card, card_state.proto, &c_apdu, &r_apdu) == -1)
                printf("ttl_io failed\n");
            else {
                printf("ttl_io finished ok\n");            
                print_r_apdu(NULL, &r_apdu);
            }
            
        } else if (card_state.proto == SCARD_PROTOCOL_T1) {
        }
        
        SCardDisconnect(card, DEFAULT_DISCONNECT_ACTION);
        break;
    }
    
    SCardReleaseContext(ctx);
    return 0;
}

int
main(int argc, char **argv) {
    int rc;
    proc_args(argc, argv);

    rc = sctest();
    
    return rc == 0 ? 0 : 1;
}
