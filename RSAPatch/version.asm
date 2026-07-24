default rel

section .text

extern OriginalFuncs_version

global GetFileVersionInfoA
global GetFileVersionInfoExA
global GetFileVersionInfoExW
global GetFileVersionInfoSizeA
global GetFileVersionInfoSizeExA
global GetFileVersionInfoSizeExW
global GetFileVersionInfoSizeW
global GetFileVersionInfoW
global VerFindFileA
global VerFindFileW
global VerInstallFileA
global VerInstallFileW
global VerLanguageNameA
global VerLanguageNameW
global VerQueryValueA
global VerQueryValueW

GetFileVersionInfoA:
    jmp [rel OriginalFuncs_version + 0*8]

GetFileVersionInfoExA:
    jmp [rel OriginalFuncs_version + 1*8]

GetFileVersionInfoExW:
    jmp [rel OriginalFuncs_version + 2*8]

GetFileVersionInfoSizeA:
    jmp [rel OriginalFuncs_version + 3*8]

GetFileVersionInfoSizeExA:
    jmp [rel OriginalFuncs_version + 4*8]

GetFileVersionInfoSizeExW:
    jmp [rel OriginalFuncs_version + 5*8]

GetFileVersionInfoSizeW:
    jmp [rel OriginalFuncs_version + 6*8]

GetFileVersionInfoW:
    jmp [rel OriginalFuncs_version + 7*8]

VerFindFileA:
    jmp [rel OriginalFuncs_version + 8*8]

VerFindFileW:
    jmp [rel OriginalFuncs_version + 9*8]

VerInstallFileA:
    jmp [rel OriginalFuncs_version + 10*8]

VerInstallFileW:
    jmp [rel OriginalFuncs_version + 11*8]

VerLanguageNameA:
    jmp [rel OriginalFuncs_version + 12*8]

VerLanguageNameW:
    jmp [rel OriginalFuncs_version + 13*8]

VerQueryValueA:
    jmp [rel OriginalFuncs_version + 14*8]

VerQueryValueW:
    jmp [rel OriginalFuncs_version + 15*8]