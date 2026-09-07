/*
 * Qcma's fixed-purpose WinUSB installer for the PS Vita.
 *
 * libwdi prepares and signs the driver package.  This small wrapper stages it
 * with SetupAPI so Windows returns the published oemNN.inf name, which Qcma
 * records to remove exactly that package on uninstall.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <newdev.h>
#include <setupapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libwdi.h"

#define QCMA_VITA_HARDWARE_ID "USB\\VID_054C&PID_04E4"
#define QCMA_VITA_INF_NAME "qcma-vita.inf"
#define QCMA_UNINSTALL_KEY "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Qcma"
#define QCMA_DRIVER_VALUE "WinUsbDriverInf"

static int is_published_inf_name(const char* published_name)
{
    size_t index, name_length;

    name_length = strlen(published_name);
    if (_strnicmp(published_name, "oem", 3) != 0 || name_length < 8 ||
        _stricmp(published_name + name_length - 4, ".inf") != 0 ||
        name_length >= MAX_PATH) return 0;
    for (index = 3; index < name_length - 4; ++index) {
        if (published_name[index] < '0' || published_name[index] > '9') return 0;
    }
    return 1;
}

static int record_published_inf(const char* published_path)
{
    const char* name = strrchr(published_path, '\\');
    HKEY key;
    LONG status;

    name = (name == NULL) ? published_path : name + 1;
    if (!is_published_inf_name(name)) {
        fprintf(stderr, "SetupAPI returned an invalid published INF name: %s\n", name);
        return 1;
    }
    status = RegCreateKeyExA(HKEY_LOCAL_MACHINE, QCMA_UNINSTALL_KEY, 0, NULL,
        0, KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &key, NULL);
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "Could not open QCMA's registry key (error %ld).\n", status);
        return 1;
    }
    status = RegSetValueExA(key, QCMA_DRIVER_VALUE, 0, REG_SZ,
        (const BYTE*)name, (DWORD)strlen(name) + 1);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "Could not record the published INF (error %ld).\n", status);
        return 1;
    }
    return 0;
}

static int remove_recorded_driver(void)
{
    HKEY key;
    char published_name[MAX_PATH];
    DWORD type = REG_SZ, size = sizeof(published_name), exit_code;
    LONG status;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process = { 0 };
    char command[MAX_PATH * 2];
    char pnputil[MAX_PATH];

    ZeroMemory(&startup, sizeof(startup));
    startup.cb = sizeof(startup);
    status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, QCMA_UNINSTALL_KEY, 0,
        KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY, &key);
    if (status == ERROR_FILE_NOT_FOUND) return 0;
    if (status != ERROR_SUCCESS) {
        fprintf(stderr, "Could not open QCMA's registry key (error %ld).\n", status);
        return 1;
    }
    status = RegQueryValueExA(key, QCMA_DRIVER_VALUE, NULL, &type,
        (BYTE*)published_name, &size);
    if (status == ERROR_FILE_NOT_FOUND) {
        RegCloseKey(key);
        return 0;
    }
    if (status != ERROR_SUCCESS || type != REG_SZ || !is_published_inf_name(published_name)) {
        fprintf(stderr, "QCMA's recorded driver package is invalid.\n");
        RegCloseKey(key);
        return 1;
    }
    snprintf(pnputil, sizeof(pnputil), "%s\\System32\\pnputil.exe", getenv("WINDIR"));
    snprintf(command, sizeof(command), "\"%s\" /delete-driver %s /uninstall", pnputil, published_name);
    if (!CreateProcessA(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW,
            NULL, NULL, &startup, &process)) {
        fprintf(stderr, "Could not start pnputil (error %lu).\n", GetLastError());
        RegCloseKey(key);
        return 1;
    }
    WaitForSingleObject(process.hProcess, INFINITE);
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    if (exit_code != 0) {
        fprintf(stderr, "pnputil could not remove %s (exit code %lu).\n", published_name, exit_code);
        RegCloseKey(key);
        return 1;
    }
    RegDeleteValueA(key, QCMA_DRIVER_VALUE);
    RegCloseKey(key);
    return 0;
}

int __cdecl main(int argc, char** argv)
{
    struct wdi_device_info device = { 0 };
    struct wdi_options_prepare_driver options = { 0 };
    char inf_path[MAX_PATH];
    char published_path[MAX_PATH];
    BOOL reboot_required = FALSE;
    DWORD error;
    int result;

    if (argc == 2 && strcmp(argv[1], "remove") == 0) {
        return remove_recorded_driver();
    }
    if (argc != 3 || strcmp(argv[1], "install") != 0) {
        fprintf(stderr, "Usage: %s install <driver-directory> | %s remove\n", argv[0], argv[0]);
        return 2;
    }
    if (snprintf(inf_path, sizeof(inf_path), "%s\\%s", argv[2], QCMA_VITA_INF_NAME) < 0) {
        fprintf(stderr, "The driver directory path is too long.\n");
        return 2;
    }

    device.vid = 0x054c;
    device.pid = 0x04e4;
    device.desc = "PS Vita";
    options.driver_type = WDI_WINUSB;
    result = wdi_prepare_driver(&device, argv[2], QCMA_VITA_INF_NAME, &options);
    if (result != WDI_SUCCESS) {
        fprintf(stderr, "libwdi could not prepare the PS Vita driver (error %d).\n", result);
        return result;
    }

    if (!SetupCopyOEMInfA(inf_path, NULL, SPOST_PATH, 0, published_path,
            sizeof(published_path), NULL, NULL)) {
        fprintf(stderr, "SetupCopyOEMInf failed (error %lu).\n", GetLastError());
        return 1;
    }
    if (record_published_inf(published_path) != 0) {
        return 1;
    }

    if (!UpdateDriverForPlugAndPlayDevicesA(NULL, QCMA_VITA_HARDWARE_ID,
            inf_path, INSTALLFLAG_FORCE, &reboot_required)) {
        error = GetLastError();
        // The package was staged successfully; a disconnected Vita simply has
        // no present device to update and will use it when connected.
        if (error != ERROR_NO_SUCH_DEVINST) {
            fprintf(stderr, "UpdateDriverForPlugAndPlayDevices failed (error %lu).\n", error);
            return 1;
        }
    }
    printf("PublishedInf=%s\n", published_path);
    return 0;
}
