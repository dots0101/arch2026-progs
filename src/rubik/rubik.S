# Directly authored RV32I, not compiler output. AI assistance is disclosed.
# Radius-eight H48 database + an on-target prefix search of depth at most 3.
# The two implementations own separate copies of all immutable tables.
    .option norvc
    .text
    .include "src/rubik/rubik_search.inc"
    .include "src/rubik/rubik_canon.inc"
    .include "src/rubik/rubik_tables.inc"
    .section .note.GNU-stack,"",@progbits
