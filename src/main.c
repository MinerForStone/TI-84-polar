#include <ti/screen.h>
#include <ti/getkey.h>
#include <ti/real.h>
#include <string.h>

struct polar_t
{
    real_t magnitude;
    real_t angle;
};

typedef struct polar_t polar_t;

struct complex_t
{
    real_t real;
    real_t imag;
};

typedef struct complex_t complex_t;

real_t r_0, r_1, r_n1, r_pi;
polar_t p_0, p_n1;

void init_consts()
{
    r_0 = os_Int24ToReal(0);
    r_1 = os_Int24ToReal(1);
    r_n1 = os_Int24ToReal(-1);
    r_pi = os_RealAcosRad(&r_n1);

    p_0 = (polar_t){r_0, r_0};
    p_n1 = (polar_t){r_1, os_RealRadToDeg(&r_pi)};
}

polar_t polarMul(const polar_t* arg1, const polar_t* arg2)
{
    polar_t result;

    result.magnitude = os_RealMul(&arg1->magnitude, &arg2->magnitude);
    result.angle = os_RealAdd(&arg1->angle, &arg2->angle);

    return result;
}

polar_t polarDiv(const polar_t* arg1, const polar_t* arg2)
{
    polar_t result;

    result.magnitude = os_RealDiv(&arg1->magnitude, &arg2->magnitude);
    result.angle = os_RealSub(&arg1->angle, &arg2->angle);

    return result;
}

complex_t polar2complex(const polar_t* arg)
{
    complex_t result;
    const real_t rad = os_RealDegToRad(&arg->angle);

    // Compute real
    result.real = os_RealCosRad(&rad);
    result.real = os_RealMul(&result.real, &arg->magnitude);

    // Compute imaginary
    result.imag = os_RealSinRad(&rad);
    result.imag = os_RealMul(&result.imag, &arg->magnitude);

    return result;
}

polar_t complex2polar(const complex_t* arg)
{
    polar_t result;

    // Compute magnitude
    const real_t real_square = os_RealMul(&arg->real, &arg->real);
    const real_t imag_square = os_RealMul(&arg->imag, &arg->imag);
    result.magnitude = os_RealAdd(&real_square, &imag_square);
    result.magnitude = os_RealSqrt(&result.magnitude);

    // Compute angle
    if (os_RealCompare(&arg->real, &r_0) == 0)
    {
        switch (os_RealCompare(&arg->imag, &r_0))
        {
        case -1:
            result.angle = os_Int24ToReal(-90);
            break;
        case 0:
            result.angle = r_0;
            break;
        case 1:
            result.angle = os_Int24ToReal(90);
            break;
            default: ;
        }
    }
    else
    {
        const real_t atan_ratio = os_RealDiv(&arg->imag, &arg->real);
        result.angle = os_RealAtanRad(&atan_ratio);
        result.angle = os_RealRadToDeg(&result.angle);
    }
    if (os_RealCompare(&arg->real, &r_0) == -1)
    {
        result = polarMul(&result, &p_n1);
    }
    return result;
}

polar_t polarAdd(const polar_t* arg1, const polar_t* arg2)
{
    complex_t result;

    const complex_t carg1 = polar2complex(arg1);
    const complex_t carg2 = polar2complex(arg2);
    result.real = os_RealAdd(&carg1.real, &carg2.real);
    result.imag = os_RealAdd(&carg1.imag, &carg2.imag);

    return complex2polar(&result);
}

polar_t polarSub(const polar_t* arg1, const polar_t* arg2)
{
    const polar_t narg2 = {os_RealNeg(&arg2->magnitude), arg2->angle};
    return polarAdd(arg1, &narg2);
}

unsigned int polarToStr(char* result, const polar_t* arg, int8_t maxLength, uint8_t mode, int8_t digits)
{
    char mag_str[100];
    char ang_str[100];
    os_RealToStr(mag_str, &arg->magnitude, maxLength, mode, digits);
    os_RealToStr(ang_str, &arg->angle, maxLength, mode, digits);

    strcpy(result, mag_str);
    strcat(result, "\x14");
    strcat(result, ang_str);

    return strlen(result);
}

real_t magnitude(const polar_t* arg)
{
    return os_RealCompare(&arg->magnitude, &r_0) == -1 ? os_RealNeg(&arg->magnitude) : arg->magnitude;
}

int magnitudeCompare(const polar_t* arg1, const polar_t* arg2)
{
    const real_t len1 = magnitude(arg1);
    const real_t len2 = magnitude(arg2);
    return os_RealCompare(&len1, &len2);
}

void print(const char* str, const uint8_t line)
{
    os_SetCursorPos(line, 0);
    os_PutStrLine("                                      ");
    os_SetCursorPos(line, 0);
    os_PutStrLine(str);
}

bool contains(const unsigned char* from, const uint8_t count, const unsigned char what)
{
    for (int i = 0; i < count; i++)
    {
        if (what == from[i])
            return true;
    }
    return false;
}

unsigned char map(const unsigned char* from, const unsigned char* to, const uint8_t count, const unsigned char what)
{
    for (int i = 0; i < count; i++)
    {
        if (what == from[i])
            return to[i];
    }
    return '?';
}

polar_t parseValue(const char* expr)
{
    polar_t value;
    char mag_buf[100];
    char ang_buf[100];
    uint8_t ang_start = 0;

    bool is_ang = false;

    // Separate magnitude and angle
    int i = 0;
    for (; expr[i] != '\0'; i++)
    {
        if (is_ang)
        {
            ang_buf[i - ang_start] = expr[i];
        }
        else if (expr[i] == '\x14')
        {
            // Check for angle sign
            ang_start = i + 1;
            mag_buf[i] = '\0';
            is_ang = true;
        }
        else
        {
            mag_buf[i] = expr[i];
        }
    }

    if (is_ang)
    {
        ang_buf[i - ang_start] = '\0';

        value.magnitude = os_StrToReal(mag_buf, NULL);
        value.angle = os_StrToReal(ang_buf, NULL);
    }
    else
    {
        mag_buf[i] = '\0';

        value.magnitude = os_StrToReal(mag_buf, NULL);
        value.angle = os_Int24ToReal(0);
    }

    return value;
}

#define STACK_SIZE 9
#define LAST_LINE 9
#define BLANK_INPUT "0"

#define BINARY_OP(k, function)\
if (key == k && stack_idx > 1)\
{\
    stack[stack_idx - 2] = function(&stack[stack_idx - 2], &stack[stack_idx -1]);\
    stack_idx--;\
}

int main()
{
    init_consts();
    os_ClrHome();

    const uint8_t valid_operator_keys[] = {k_Enter, k_Add, k_Sub, k_Mul, k_Div, k_Clear, k_Del};
    const uint8_t valid_char_keys[] = {k_0, k_1, k_2, k_3, k_4, k_5, k_6, k_7, k_8, k_9, k_DecPnt, k_Chs, k_Sin};
    const unsigned char valid_chars[] = "0123456789.\x1A\x14";

    polar_t stack[STACK_SIZE];
    uint8_t stack_idx = 0;

    uint8_t key;

    while (true)
    {
        char output_buf[100];
        char input_buf[100];
        uint8_t input_idx = 0;
        input_buf[0] = '\0';
        print(BLANK_INPUT, LAST_LINE);

        while (!contains(valid_operator_keys, 7, key = (uint8_t)os_GetKey()))
        {
            if (key == k_Quit)
                return 0;

            if (contains(valid_char_keys, 13, key))
            {
                input_buf[input_idx++] = (char)map(valid_char_keys, valid_chars, 13, key);
                input_buf[input_idx] = '\0';
            }

            print(input_idx ? input_buf : BLANK_INPUT, LAST_LINE);
        }

        if (key == k_Enter && stack_idx < STACK_SIZE)
        {
            stack[stack_idx++] = parseValue(input_buf);
        }

        switch (key)
        {
        case k_Clear:
            stack_idx = 0;
        case k_Del:
            input_buf[0] = '\0';
        default: ;
        }

        BINARY_OP(k_Add, polarAdd)
        BINARY_OP(k_Sub, polarSub)
        BINARY_OP(k_Mul, polarMul)
        BINARY_OP(k_Div, polarDiv)

        for (int i = 0; i < STACK_SIZE; i++)
        {
            if (i < stack_idx)
            {
                polarToStr(output_buf, &stack[i], -1, 0, -1);
                print(output_buf, i);
            }
            else
            {
                print("", i);
            }
        }
    }
}