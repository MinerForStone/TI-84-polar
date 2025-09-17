#include <ti/screen.h>
#include <ti/getkey.h>
#include <ti/getcsc.h>
#include <ti/real.h>
#include <string.h>

struct polar_t
{
    real_t magnitude;
    real_t angle;
};

typedef struct polar_t polar_t;

struct component_t
{
    real_t real;
    real_t imag;
};

typedef struct component_t component_t;

real_t r_0, r_1, r_180, r_360, r_n1, r_n180, r_pi;
polar_t p_0, p_n1;

void init_consts()
{
    r_0 = os_Int24ToReal(0);
    r_1 = os_Int24ToReal(1);
    r_180 = os_Int24ToReal(180);
    r_360 = os_Int24ToReal(360);
    r_n1 = os_Int24ToReal(-1);
    r_n180 = os_Int24ToReal(-180);
    r_pi = os_RealAcosRad(&r_n1);

    p_0 = (polar_t){r_0, r_0};
    p_n1 = (polar_t){r_1, os_RealRadToDeg(&r_pi)};
}

void normalizeAngle(polar_t *arg)
{
    if (os_RealCompare(&arg->magnitude, &r_0) == -1)
    {
        arg->angle = os_RealAdd(&arg->angle, &r_180);
        arg->magnitude = os_RealNeg(&arg->magnitude);
    }
    while (os_RealCompare(&arg->angle, &r_180) == 1)
    {
        arg->angle = os_RealSub(&arg->angle, &r_360);
    }
    while (os_RealCompare(&arg->angle, &r_n180) != 1)
    {
        arg->angle = os_RealAdd(&arg->angle, &r_360);
    }
}

polar_t polarMul(const polar_t* arg1, const polar_t* arg2)
{
    polar_t result;

    result.magnitude = os_RealMul(&arg1->magnitude, &arg2->magnitude);
    result.angle = os_RealAdd(&arg1->angle, &arg2->angle);

    normalizeAngle(&result);
    return result;
}

polar_t polarDiv(const polar_t* arg1, const polar_t* arg2)
{
    polar_t result;

    result.magnitude = os_RealDiv(&arg1->magnitude, &arg2->magnitude);
    result.angle = os_RealSub(&arg1->angle, &arg2->angle);

    normalizeAngle(&result);
    return result;
}

component_t polar2component(const polar_t* arg)
{
    component_t result;
    const real_t rad = os_RealDegToRad(&arg->angle);

    // Compute real
    result.real = os_RealCosRad(&rad);
    result.real = os_RealMul(&result.real, &arg->magnitude);

    // Compute imaginary
    result.imag = os_RealSinRad(&rad);
    result.imag = os_RealMul(&result.imag, &arg->magnitude);

    return result;
}

polar_t component2polar(const component_t* arg)
{
    polar_t result;

    // Compute magnitude
    const real_t real_square = os_RealMul(&arg->real, &arg->real);
    const real_t imag_square = os_RealMul(&arg->imag, &arg->imag);
    result.magnitude = os_RealAdd(&real_square, &imag_square);
    result.magnitude = os_RealCompare(&result.magnitude, &r_0) == 0 ? r_0 : os_RealSqrt(&result.magnitude);

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
        result.angle = os_RealAdd(&result.angle, &r_180);
    }

    normalizeAngle(&result);
    return result;
}

polar_t polarAdd(const polar_t* arg1, const polar_t* arg2)
{
    component_t result;

    const component_t c_arg1 = polar2component(arg1);
    const component_t c_arg2 = polar2component(arg2);
    result.real = os_RealAdd(&c_arg1.real, &c_arg2.real);
    result.imag = os_RealAdd(&c_arg1.imag, &c_arg2.imag);

    return component2polar(&result);
}

polar_t polarSub(const polar_t* arg1, const polar_t* arg2)
{
    const polar_t narg2 = {os_RealNeg(&arg2->magnitude), arg2->angle};
    return polarAdd(arg1, &narg2);
}

polar_t polarExpon(const polar_t* arg1, const polar_t* arg2)
{
    const component_t c_arg2 = polar2component(arg2);

    real_t exp1 = os_RealLog(&arg1->magnitude);
    exp1 = os_RealMul(&c_arg2.real, &exp1);
    real_t exp1a = os_RealDegToRad(&arg1->angle);
    exp1a = os_RealMul(&c_arg2.imag, &exp1a);
    exp1 = os_RealSub(&exp1, &exp1a);

    real_t exp2 = os_RealLog(&arg1->magnitude);
    exp2 = os_RealMul(&c_arg2.imag, &exp2);
    real_t exp2a = os_RealDegToRad(&arg1->angle);
    exp2a = os_RealMul(&c_arg2.real, &exp2a);
    exp2 = os_RealAdd(&exp2, &exp2a);

    polar_t fact1 = {os_RealExp(&exp1), r_0};
    polar_t fact2 = {r_1, os_RealRadToDeg(&exp2)};

    return polarMul(&fact1, &fact2);
}

polar_t polarInsertAngle(const polar_t* magnitude, const polar_t* angle)
{
    polar_t result;

    result.magnitude = magnitude->magnitude;
    result.angle = angle->magnitude;

    return result;
}

unsigned int polarToStr(char* result, const polar_t* arg, int8_t maxLength, uint8_t mode, int8_t digits, bool asComponents)
{
    if (!asComponents)
    {
        char mag_str[100];
        char ang_str[100];
        os_RealToStr(mag_str, &arg->magnitude, maxLength, mode, digits);
        os_RealToStr(ang_str, &arg->angle, maxLength, mode, digits);

        strcpy(result, mag_str);
        strcat(result, "\x14");
        strcat(result, ang_str);
    }
    else
    {
        component_t c_arg = polar2component(arg);
        bool sub_imag = false;
        if (os_RealCompare(&c_arg.imag, &r_0) == -1)
        {
            c_arg.imag = os_RealNeg(&c_arg.imag);
            sub_imag = true;
        }

        char real_str[100];
        char imag_str[100];
        os_RealToStr(real_str, &c_arg.real, maxLength, mode, digits);
        os_RealToStr(imag_str, &c_arg.imag, maxLength, mode, digits);

        strcpy(result, real_str);
        strcat(result, sub_imag ? "-" : "+");
        strcat(result, imag_str);
        strcat(result, "i");
    }

    return strlen(result);
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
    char buf1[100];
    char buf2[100];
    uint8_t buf2_start = 0;

    bool is_ang = false;
    bool is_imag = false;

    // Separate magnitude and angle
    int i = 0;
    for (; expr[i] != '\0'; i++)
    {
        if (is_ang || is_imag)
        {
            buf2[i - buf2_start] = expr[i];
        }
        else if (expr[i] == '\x14')
        {
            // Check for angle sign
            buf2_start = i + 1;
            buf1[i] = '\0';
            is_ang = true;
        }
        else if (expr[i] == ',')
        {
            // Check for comma
            buf2_start = i + 1;
            buf1[i] = '\0';
            is_imag = true;
        }
        else
        {
            buf1[i] = expr[i];
        }
    }

    if (is_ang)
    {
        buf2[i - buf2_start] = '\0';

        polar_t value;
        value.magnitude = os_StrToReal(buf1, NULL);
        value.angle = os_StrToReal(buf2, NULL);

        normalizeAngle(&value);
        return value;
    }
    if (is_imag)
    {
        buf2[i - buf2_start] = '\0';

        component_t value;
        value.real = os_StrToReal(buf1, NULL);
        value.imag = os_StrToReal(buf2, NULL);

        polar_t p_value = component2polar(&value);
        normalizeAngle(&p_value);
        return p_value;
    }

    buf1[i] = '\0';

    polar_t value;
    value.magnitude = os_StrToReal(buf1, NULL);
    value.angle = r_0;

    normalizeAngle(&value);
    return value;
}

#define STACK_SIZE 9
#define LAST_LINE 9
#define BLANK_INPUT "0"
#define OPERATOR_COUNT 10
#define CHAR_COUNT 14

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

    const uint8_t valid_operator_keys[] = {k_Enter, k_Add, k_Sub, k_Mul, k_Div, k_Clear, k_Del, k_Mode, k_Cos, k_Expon};
    const uint8_t valid_char_keys[] = {k_0, k_1, k_2, k_3, k_4, k_5, k_6, k_7, k_8, k_9, k_Comma, k_DecPnt, k_Chs, k_Sin};
    const unsigned char valid_chars[] = "0123456789,.\x1A\x14";

    polar_t stack[STACK_SIZE];
    uint8_t stack_idx = 0;

    bool componentsMode = false;

    uint8_t key;

    while (true)
    {
        char output_buf[100];
        char input_buf[100];
        uint8_t input_idx = 0;
        input_buf[0] = '\0';
        print(BLANK_INPUT, LAST_LINE);

        while (!contains(valid_operator_keys, OPERATOR_COUNT, key = (uint8_t)os_GetKey()))
        {
            if (key == k_Quit || boot_CheckOnPressed())
                return 0;

            if (contains(valid_char_keys, CHAR_COUNT, key))
            {
                input_buf[input_idx++] = (char)map(valid_char_keys, valid_chars, CHAR_COUNT, key);
                input_buf[input_idx] = '\0';
            }

            print(input_idx > 0 ? input_buf : BLANK_INPUT, LAST_LINE);
        }

        if (key == k_Mode)
            componentsMode = !componentsMode;

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
        BINARY_OP(k_Cos, polarInsertAngle)
        BINARY_OP(k_Expon, polarExpon)

        for (int i = 0; i < STACK_SIZE; i++)
        {
            if (i < stack_idx)
            {
                polarToStr(output_buf, &stack[i], -1, 0, -1, componentsMode);
                print(output_buf, i);
            }
            else
            {
                print("", i);
            }
        }
    }
}