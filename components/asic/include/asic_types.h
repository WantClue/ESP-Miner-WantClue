#ifndef ASIC_TYPES_H
#define ASIC_TYPES_H

typedef enum {
    ASIC_UNKNOWN = -1,
    ASIC_BM1397,
    ASIC_BM1366,
    ASIC_BM1368,
    ASIC_BM1370,
} AsicModel;

typedef AsicModel asic_type_t;

#endif /* ASIC_TYPES_H */