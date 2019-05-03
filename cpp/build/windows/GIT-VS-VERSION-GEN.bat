@ECHO OFF
SETLOCAL

REM  Script for generation of rc VERSIONINFO & StringFileInfo

REM ====================
REM Installation Variables
REM ====================
:: VERSION_FILE - Untracked file to be included in packaged source releases.
::                it should contain a single line in the format:
::                $Project_Name VERSION $tag (ie: Foobar VERSION v1.0.0-alpha0)
SET VERSION_FILE=GIT-VS-VERSION-FILE

:: DEFAULT_VERSION - Version string to be processed when neither Git nor a
::                packed version file is available.
SET DEFAULT_VERSION=v1.6.0-NoGit

:: COUNT_PATCHES_FROM - Determines which tag to count the number of patches from
::                for the final portion of the digital version number.
::                Valid values are:
::                   major - count from earliest Major.0.0* tag.
::                   minor - count from earliest Major.Minor.0* tag.
::                   fix   - count from earliest Major.Minor.Fix tag.
SET COUNT_PATCHES_FROM=fix

:: USES_PRERELEASE_TAGS - numeric bool value to determine if GET_GIT_PATCHES
::                function should read the number of patches in the format of
::                  (Default) 1 - Major.Minor.Fix-Stage#-'CommitCount'
::                            0 - Major.Minor.Fix-'CommitCount'
SET USE_PRERELEASE_TAGS=1

:: --------------------
:CHECK_ARGS
:: --------------------

:: Console output only.
IF [%1] == [] GOTO START

IF "%~1" == "--help" GOTO USAGE
IF "%~1" == "--quiet" SET fQUIET=1& SHIFT
IF "%~1" == "--force" SET fFORCE=1& SHIFT

:: Un-documented switch
IF "%~1" == "--test" GOTO TEST

IF EXIST %~1\NUL (
  :: %1 is a path
  SET CACHE_FILE=%~s1\%VERSION_FILE%
  SHIFT
)

IF [%~nx1] NEQ [] (
  :: %1 is a file
  SET HEADER_OUT_FILE=%~fs1
  SHIFT
)
:: This should always be the last argument.
IF [%1] NEQ [] GOTO USAGE

:: Some basic sanity checks.
IF DEFINED fQUIET (
  IF NOT DEFINED HEADER_OUT_FILE GOTO USAGE
)

IF DEFINED CACHE_FILE (
  SET CACHE_FILE=%CACHE_FILE:\\=\%
  IF NOT DEFINED HEADER_OUT_FILE GOTO USAGE
)
GOTO START

:: --------------------
:USAGE
:: --------------------
ECHO usage: [--help] ^| ^| [--quiet] [--force] [CACHE PATH] [OUT FILE]
ECHO.
ECHO  When called without arguments version information writes to console.
ECHO.
ECHO  --help      - displays this output.
ECHO.
ECHO  --quiet     - Suppress console output.
ECHO  --force     - Ignore cached version information.
ECHO  CACHE PATH  - Path for non-tracked file to store git-describe version.
ECHO  OUT FILE    - Path to writable file that is included in the project's rc file.
ECHO.
ECHO  Version information is expected to be in the format: vMajor.Minor.Fix[-stage#]
ECHO  Where -stage# is alpha, beta, or rc. ( example: v1.0.0-alpha0 )
ECHO.
ECHO  Example pre-build event:
ECHO  CALL $(SolutionDir)..\scripts\GIT-VS-VERSION-GEN.bat "$(IntDir)\" "$(SolutionDir)..\src\gen-versioninfo.h"
ECHO.
GOTO END


REM ===================
REM Entry Point
REM ===================
:START
ECHO.
CALL :INIT_VARS
CALL :GET_VERSION_STRING
IF DEFINED fGIT_AVAILABLE (
  IF DEFINED fLEAVE_NOW GOTO END
  IF DEFINED CACHE_FILE (
    CALL :CHECK_CACHE
  )
)
IF DEFINED fLEAVE_NOW GOTO END
CALL :SET_BUILD_PARTS
CALL :PREP_OUT
CALL :WRITE_OUT
GOTO END

REM ====================
REM FUNCTIONS
REM ====================
:: --------------------
:INIT_VARS
:: --------------------
:: The following variables are used for the final version output.
::    String Version:  Major.Minor.Fix.Stage#[.Patches.SHA1[.dirty]]
SET strFILE_VERSION=

::    Digital Version: Major, Minor, Fix, Patches
SET nbMAJOR_PART=0
SET nbMINOR_PART=0
SET nbFIX_PART=0
SET nbPATCHES_PART=0

:: VERSIONINFO VS_FF_ flags
SET fPRIVATE=0
SET fPATCHED=0
SET fPRE_RELEASE=0

:: Supporting StringFileInfo - not used for clean release builds.
SET strPRIVATE_BUILD=
SET strCOMMENT=

GOTO :EOF

:: --------------------
:GET_VERSION_STRING
:: --------------------
:: Precedence is Git, VERSION_FILE, then DEFAULT_VERSION.
:: Check if git is available by testing git describe.
CALL git describe>NUL 2>&1
IF NOT ERRORLEVEL 1 (
  SET fGIT_AVAILABLE=1
  :: Parse git version string
  CALL :PARSE_GIT_STRING
) ELSE (
  :: Use the VERSION_FILE if it exists.
  IF EXIST "%VERSION_FILE%" (
    FOR /F "tokens=3" %%A IN (%VERSION_FILE%) DO (
      SET strFILE_VERSION=%%A
    )
  ) ELSE (
    :: Default to the DEFAULT_VERSION
    SET strFILE_VERSION=%DEFAULT_VERSION%
  )
)
SET strFILE_VERSION=%strFILE_VERSION:~1%
SET strFILE_VERSION=%strFILE_VERSION:-=.%
GOTO :EOF

:: --------------------
:PARSE_GIT_STRING
:: --------------------
FOR /F "tokens=*" %%A IN ('"git describe --long --tags --dirty "') DO (
  SET strFILE_VERSION=%%A
)
echo %strFILE_VERSION%
:: If HEAD is dirty then this is not part of an official build and even if a
:: commit hasn't been made it should still be marked as dirty and patched.
SET tmp=
GOTO :EOF

:: --------------------
:CHECK_CACHE
:: --------------------
:: Exit early if a cached git built version matches the current version.
IF DEFINED HEADER_OUT_FILE (
  IF EXIST "%HEADER_OUT_FILE%" (
    IF [%fFORCE%] EQU [1] DEL "%CACHE_FILE%"
    IF EXIST "%CACHE_FILE%" (
      FOR /F "tokens=*" %%A IN (%CACHE_FILE%) DO (
        IF "%%A" == "%strFILE_VERSION%" (
          IF NOT DEFINED fQUIET (
            ECHO Build version is assumed unchanged from: %strFILE_VERSION%.
          )
          SET fLEAVE_NOW=1
        )
      )
    )
  )

  ECHO %strFILE_VERSION%> "%CACHE_FILE%"
)
GOTO :EOF

:: --------------------
:SET_BUILD_PARTS
:: --------------------
:: The min version is X.Y.Z and the max is X.Y.Z.Stage#.Commits.SHA.dirty
:: strTMP_STAGE_PART is a holder for anything past 'X.Y.Z.'.
FOR /F "tokens=1,2,3,* delims=." %%A IN ("%strFile_Version%") DO (
  SET nbMAJOR_PART=%%A
  SET nbMINOR_PART=%%B
  SET nbFIX_PART=%%C
  SET strTMP_STAGE_PART=%%D
)
GOTO :EOF

:: --------------------
:PREP_OUT
:: --------------------
SET csvFILE_VERSION=%nbMAJOR_PART%,%nbMINOR_PART%,%nbFIX_PART%,%nbPATCHES_PART%
SET hexFILE_VERSION=
CALL :SET_HEX

IF NOT %fPRIVATE% EQU 0 SET fPRIVATE=VS_FF_PRIVATEBUILD
IF NOT %fPATCHED% EQU 0 SET fPATCHED=VS_FF_PATCHED
IF NOT %fPRE_RELEASE% EQU 0 SET fPRE_RELEASE=VS_FF_PRERELEASE
GOTO :EOF

:: --------------------
:SET_HEX
:: --------------------
:: Iterate Major, Minor, Fix, Patches as set in csvFILEVERSION and convert to
:: hex while appending to the hexFILE_VERION string to give a padded 32bit
:: end result. ie: v1.0.1.34 = 0x0001000000010022
SET hex_values=0123456789ABCDEF

FOR /F "tokens=1-4 delims=," %%A IN ("%csvFILE_VERSION%") DO (
  CALL :int2hex %%A
  CALL :int2hex %%B
  CALL :int2hex %%C
  CALL :int2hex %%D
)

SET hexFILE_VERSION=0x%hexFILE_VERSION%
SET hex_values=

GOTO :EOF

:int2hex
SETLOCAL ENABLEDELAYEDEXPANSION
SET /A pad=4
SET /A iVal=%1

:hex_loop
SET /A pad=%pad% - 1
SET /A hVal=%iVal% %% 16
SET hVal=!hex_values:~%hVal%,1!
SET hex_word=%hVal%%hex_word%
SET /A iVal=%iVal% / 16
IF %iVal% GTR 0 GOTO hex_loop

:hex_pad_loop
FOR /L %%A in (1,1,%pad%) DO SET hex_word=0!hex_word!
ENDLOCAL& SET hexFILE_VERSION=%hexFILE_VERSION%%hex_word%
GOTO :EOF

:: --------------------
:WRITE_OUT
:: --------------------
:: HEADER_OUT falls through to CON_OUT which checks for the QUIET flag.
IF DEFINED HEADER_OUT_FILE (
  CALL :OUT_HEADER
) ELSE (
  IF NOT DEFINED TESTING (
  CALL :CON_OUT
  ) ELSE (
    CALL :TEST_OUT
  )
)
GOTO :EOF

:: --------------------
:OUT_HEADER
:: --------------------
ECHO unsigned short ozw_vers_major = %nbMAJOR_PART%; >> "%HEADER_OUT_FILE%"
ECHO unsigned short ozw_vers_minor = %nbMINOR_PART%; >> "%HEADER_OUT_FILE%"
ECHO unsigned short ozw_vers_revision = %nbFIX_PART%; >> "%HEADER_OUT_FILE%"
ECHO char ozw_version_string[] = "%strFILE_VERSION%\0"; >> "%HEADER_OUT_FILE%"


SET nbMAJOR_PART=0
SET nbMINOR_PART=0
SET nbFIX_PART=0
SET nbPATCHES_PART=0

:: --------------------
:CON_OUT
:: --------------------
IF DEFINED fQUIET GOTO :EOF
ECHO Version String:       %strFILE_VERSION%
ECHO Digital Version ID:   %csvFILE_VERSION%
ECHO Hex Version ID:       %hexFILE_VERSION%
GOTO :EOF


:: --------------------
:END
:: --------------------
