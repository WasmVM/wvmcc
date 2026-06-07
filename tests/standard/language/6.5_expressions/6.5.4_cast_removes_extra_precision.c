/* LANG-6.5.4-03 — A cast that specifies a floating type with the same type
 * as the (possibly extended-precision) result removes any extra range and
 * precision (ISO C17 6.5.4p6). After casting to `float`/`double`, the value
 * must be exactly representable in that type, so re-casting / comparing is
 * stable and reflects the rounded value, not any wider intermediate. */

int main(void)
{
    /* 0.1 has no exact float representation. Casting an expression to float
     * forces the value into float precision; comparing two such floats that
     * were each cast to float must agree. */
    float a = (float)0.1;
    float b = (float)(1.0 / 10.0);
    if (a != b) return 1;

    /* A value cast to float must equal itself when read back as float:
     * the extra precision of the computation is discarded by the cast. */
    double wide = 1.0 / 3.0;          /* computed in double precision */
    float narrow = (float)wide;       /* extra precision removed */
    if (narrow != (float)narrow) return 2;

    /* 16777217 = 2^24 + 1 is not representable in IEEE single precision;
     * casting to float rounds it to 16777216, and the extra integer
     * precision is removed. */
    if ((float)16777217 != (float)16777216) return 3;

    /* Casting a double back through float loses precision that double had:
     * the float-cast value differs from the original double. */
    double third = 1.0 / 3.0;
    if ((double)(float)third == third) return 4;  /* must differ */

    return 0;
}
