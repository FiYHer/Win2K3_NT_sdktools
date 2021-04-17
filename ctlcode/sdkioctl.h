/*++

NOTE: The table below *MUST* be kept sorted in order for the utility
to work properly.  feel free to add your own new ioctls, but only if
you can get the tool to compile.  i had to originally exclude a few
ioctls because i was unable to determine a method which define'd them
and did not cause error messages due to redefine'd structs, typedefs,
etc.

thanks!

--*/

IOCTL_VALUE TableIoctlValue[] = {
    SeedIoctlValue(IOCTL_ABORT_PIPE),
    SeedIoctlValue(IOCTL_BATTERY_QUERY_INFORMATION),
    SeedIoctlValue(IOCTL_BATTERY_QUERY_STATUS),
    SeedIoctlValue(IOCTL_BATTERY_QUERY_TAG),
    SeedIoctlValue(IOCTL_BATTERY_SET_INFORMATION),
    SeedIoctlValue(IOCTL_BEEP_SET),
    SeedIoctlValue(IOCTL_BURNENG_BURN),
    SeedIoctlValue(IOCTL_BURNENG_INIT),
    SeedIoctlValue(IOCTL_BURNENG_PROGRESS),
    SeedIoctlValue(IOCTL_BURNENG_TERM),
    SeedIoctlValue(IOCTL_CANCEL_DEVICE_WAKE),
    SeedIoctlValue(IOCTL_CANCEL_GET_SEND_MESSAGE),
    SeedIoctlValue(IOCTL_CANCEL_IO),
    SeedIoctlValue(IOCTL_CDROM_CHECK_VERIFY),
    SeedIoctlValue(IOCTL_CDROM_DISK_TYPE),
    SeedIoctlValue(IOCTL_CDROM_EJECT_MEDIA),
    SeedIoctlValue(IOCTL_CDROM_FIND_NEW_DEVICES),
    SeedIoctlValue(IOCTL_CDROM_GET_DRIVE_GEOMETRY),
    SeedIoctlValue(IOCTL_CDROM_GET_LAST_SESSION),
    SeedIoctlValue(IOCTL_CDROM_GET_VOLUME),
    SeedIoctlValue(IOCTL_CDROM_LOAD_MEDIA),
    SeedIoctlValue(IOCTL_CDROM_MEDIA_REMOVAL),
    SeedIoctlValue(IOCTL_CDROM_PAUSE_AUDIO),
    SeedIoctlValue(IOCTL_CDROM_PLAY_AUDIO_MSF),
    SeedIoctlValue(IOCTL_CDROM_RAW_READ),
    SeedIoctlValue(IOCTL_CDROM_READ_Q_CHANNEL),
    SeedIoctlValue(IOCTL_CDROM_READ_TOC),
    SeedIoctlValue(IOCTL_CDROM_READ_TOC_EX),
    SeedIoctlValue(IOCTL_CDROM_RELEASE),
    SeedIoctlValue(IOCTL_CDROM_RESERVE),
    SeedIoctlValue(IOCTL_CDROM_RESUME_AUDIO),
    SeedIoctlValue(IOCTL_CDROM_SEEK_AUDIO_MSF),
    SeedIoctlValue(IOCTL_CDROM_SET_VOLUME),
    SeedIoctlValue(IOCTL_CDROM_SIMBAD),
    SeedIoctlValue(IOCTL_CDROM_STOP_AUDIO),
    SeedIoctlValue(IOCTL_CDROM_UNLOAD_DRIVER),
    SeedIoctlValue(IOCTL_CHANGER_EXCHANGE_MEDIUM),
    SeedIoctlValue(IOCTL_CHANGER_GET_ELEMENT_STATUS),
    SeedIoctlValue(IOCTL_CHANGER_GET_PARAMETERS),
    SeedIoctlValue(IOCTL_CHANGER_GET_PRODUCT_DATA),
    SeedIoctlValue(IOCTL_CHANGER_GET_STATUS),
    SeedIoctlValue(IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS),
    SeedIoctlValue(IOCTL_CHANGER_MOVE_MEDIUM),
    SeedIoctlValue(IOCTL_CHANGER_QUERY_VOLUME_TAGS),
    SeedIoctlValue(IOCTL_CHANGER_REINITIALIZE_TRANSPORT),
    SeedIoctlValue(IOCTL_CHANGER_SET_ACCESS),
    SeedIoctlValue(IOCTL_CHANGER_SET_POSITION),
    SeedIoctlValue(IOCTL_DISK_CHECK_VERIFY),
    SeedIoctlValue(IOCTL_DISK_CREATE_DISK),
    SeedIoctlValue(IOCTL_DISK_CONTROLLER_NUMBER),
    SeedIoctlValue(IOCTL_DISK_DELETE_DRIVE_LAYOUT),
    SeedIoctlValue(IOCTL_DISK_EJECT_MEDIA),
    SeedIoctlValue(IOCTL_DISK_FIND_NEW_DEVICES),
    SeedIoctlValue(IOCTL_DISK_FORMAT_DRIVE),
    SeedIoctlValue(IOCTL_DISK_FORMAT_TRACKS),
    SeedIoctlValue(IOCTL_DISK_FORMAT_TRACKS_EX),
    SeedIoctlValue(IOCTL_DISK_GET_CACHE_INFORMATION),
    SeedIoctlValue(IOCTL_DISK_GET_DRIVE_GEOMETRY),
    SeedIoctlValue(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX),
    SeedIoctlValue(IOCTL_DISK_GET_DRIVE_LAYOUT),
    SeedIoctlValue(IOCTL_DISK_GET_DRIVE_LAYOUT_EX),
    SeedIoctlValue(IOCTL_DISK_GET_LENGTH_INFO),
    SeedIoctlValue(IOCTL_DISK_GET_MEDIA_TYPES),
    SeedIoctlValue(IOCTL_DISK_GET_PARTITION_INFO),
    SeedIoctlValue(IOCTL_DISK_GET_PARTITION_INFO_EX),
    SeedIoctlValue(IOCTL_DISK_GROW_PARTITION),
    SeedIoctlValue(IOCTL_DISK_HISTOGRAM_DATA),
    SeedIoctlValue(IOCTL_DISK_HISTOGRAM_RESET),
    SeedIoctlValue(IOCTL_DISK_HISTOGRAM_STRUCTURE),
    SeedIoctlValue(IOCTL_DISK_INTERNAL_CLEAR_VERIFY),
    SeedIoctlValue(IOCTL_DISK_INTERNAL_SET_NOTIFY),
    SeedIoctlValue(IOCTL_DISK_INTERNAL_SET_VERIFY),
    SeedIoctlValue(IOCTL_DISK_IS_WRITABLE),
    SeedIoctlValue(IOCTL_DISK_LOAD_MEDIA),
    SeedIoctlValue(IOCTL_DISK_LOGGING),
    SeedIoctlValue(IOCTL_DISK_MEDIA_REMOVAL),
    SeedIoctlValue(IOCTL_DISK_PERFORMANCE),
    SeedIoctlValue(IOCTL_DISK_REASSIGN_BLOCKS),
    SeedIoctlValue(IOCTL_DISK_RELEASE),
    SeedIoctlValue(IOCTL_DISK_REQUEST_DATA),
    SeedIoctlValue(IOCTL_DISK_REQUEST_STRUCTURE),
    SeedIoctlValue(IOCTL_DISK_RESERVE),
    SeedIoctlValue(IOCTL_DISK_SENSE_DEVICE),
    SeedIoctlValue(IOCTL_DISK_SET_CACHE_INFORMATION),
    SeedIoctlValue(IOCTL_DISK_SET_DRIVE_LAYOUT),
    SeedIoctlValue(IOCTL_DISK_SET_DRIVE_LAYOUT_EX),
    SeedIoctlValue(IOCTL_DISK_SET_PARTITION_INFO),
    SeedIoctlValue(IOCTL_DISK_SET_PARTITION_INFO_EX),
    SeedIoctlValue(IOCTL_DISK_SIMBAD),
    SeedIoctlValue(IOCTL_DISK_UPDATE_DRIVE_SIZE),
    SeedIoctlValue(IOCTL_DISK_VERIFY),
    SeedIoctlValue(IOCTL_DLC_BUFFER_CREATE),
    SeedIoctlValue(IOCTL_DLC_BUFFER_FREE),
    SeedIoctlValue(IOCTL_DLC_BUFFER_GET),
    SeedIoctlValue(IOCTL_DLC_CLOSE_ADAPTER),
    SeedIoctlValue(IOCTL_DLC_CLOSE_DIRECT),
    SeedIoctlValue(IOCTL_DLC_CLOSE_SAP),
    SeedIoctlValue(IOCTL_DLC_CLOSE_STATION),
    SeedIoctlValue(IOCTL_DLC_COMPLETE_COMMAND),
    SeedIoctlValue(IOCTL_DLC_CONNECT_STATION),
    SeedIoctlValue(IOCTL_DLC_FLOW_CONTROL),
    SeedIoctlValue(IOCTL_DLC_LAST_COMMAND),
    SeedIoctlValue(IOCTL_DLC_MAX),
    SeedIoctlValue(IOCTL_DLC_OPEN_ADAPTER),
    SeedIoctlValue(IOCTL_DLC_OPEN_DIRECT),
    SeedIoctlValue(IOCTL_DLC_OPEN_SAP),
    SeedIoctlValue(IOCTL_DLC_OPEN_STATION),
    SeedIoctlValue(IOCTL_DLC_QUERY_INFORMATION),
    SeedIoctlValue(IOCTL_DLC_READ),
    SeedIoctlValue(IOCTL_DLC_READ_CANCEL),
    SeedIoctlValue(IOCTL_DLC_READ2),
    SeedIoctlValue(IOCTL_DLC_REALLOCTE_STATION),
    SeedIoctlValue(IOCTL_DLC_RECEIVE),
    SeedIoctlValue(IOCTL_DLC_RECEIVE_CANCEL),
    SeedIoctlValue(IOCTL_DLC_RECEIVE2),
    SeedIoctlValue(IOCTL_DLC_RESET),
    SeedIoctlValue(IOCTL_DLC_SET_EXCEPTION_FLAGS),
    SeedIoctlValue(IOCTL_DLC_SET_INFORMATION),
    SeedIoctlValue(IOCTL_DLC_TIMER_CANCEL),
    SeedIoctlValue(IOCTL_DLC_TIMER_CANCEL_GROUP),
    SeedIoctlValue(IOCTL_DLC_TIMER_SET),
    SeedIoctlValue(IOCTL_DLC_TRACE_INITIALIZE),
    SeedIoctlValue(IOCTL_DLC_TRANSMIT),
    SeedIoctlValue(IOCTL_DLC_TRANSMIT2),
    SeedIoctlValue(IOCTL_DVD_END_SESSION),
    SeedIoctlValue(IOCTL_DVD_GET_REGION),
    SeedIoctlValue(IOCTL_DVD_READ_KEY),
    SeedIoctlValue(IOCTL_DVD_READ_STRUCTURE),
    SeedIoctlValue(IOCTL_DVD_SEND_KEY),
    SeedIoctlValue(IOCTL_DVD_SEND_KEY2),
    SeedIoctlValue(IOCTL_DVD_SET_READ_AHEAD),
    SeedIoctlValue(IOCTL_DVD_START_SESSION),
    SeedIoctlValue(IOCTL_FSVIDEO_COPY_FRAME_BUFFER),
    SeedIoctlValue(IOCTL_FSVIDEO_REVERSE_MOUSE_POINTER),
    SeedIoctlValue(IOCTL_FSVIDEO_SET_CURRENT_MODE),
    SeedIoctlValue(IOCTL_FSVIDEO_SET_CURSOR_POSITION),
    SeedIoctlValue(IOCTL_FSVIDEO_SET_SCREEN_INFORMATION),
    SeedIoctlValue(IOCTL_FSVIDEO_WRITE_TO_FRAME_BUFFER),
    SeedIoctlValue(IOCTL_GAMEENUM_ACQUIRE_ACCESSORS),
    SeedIoctlValue(IOCTL_GAMEENUM_EXPOSE_HARDWARE),
    SeedIoctlValue(IOCTL_GAMEENUM_EXPOSE_SIBLING),
    SeedIoctlValue(IOCTL_GAMEENUM_PORT_DESC),
    SeedIoctlValue(IOCTL_GAMEENUM_PORT_PARAMETERS),
    SeedIoctlValue(IOCTL_GAMEENUM_REMOVE_HARDWARE),
    SeedIoctlValue(IOCTL_GAMEENUM_REMOVE_SELF),
    SeedIoctlValue(IOCTL_GET_CHANNEL_ALIGN_RQST),
    SeedIoctlValue(IOCTL_GET_DEVICE_DESCRIPTOR),
    SeedIoctlValue(IOCTL_GET_HCD_DRIVERKEY_NAME),
    SeedIoctlValue(IOCTL_GET_NUM_DEVICE_INPUT_BUFFERS),
    SeedIoctlValue(IOCTL_GET_PHYSICAL_DESCRIPTOR),
    SeedIoctlValue(IOCTL_GET_PIPE_CONFIGURATION),
    SeedIoctlValue(IOCTL_GET_TUPLE_DATA),
    SeedIoctlValue(IOCTL_GET_USB_DESCRIPTOR),
    SeedIoctlValue(IOCTL_GET_VERSION),
    SeedIoctlValue(IOCTL_HID_FLUSH_QUEUE),
    SeedIoctlValue(IOCTL_HID_GET_COLLECTION_DESCRIPTOR),
    SeedIoctlValue(IOCTL_HID_GET_COLLECTION_INFORMATION),
    SeedIoctlValue(IOCTL_HID_GET_DRIVER_CONFIG),
    SeedIoctlValue(IOCTL_HID_GET_FEATURE),
    SeedIoctlValue(IOCTL_HID_GET_HARDWARE_ID),
    SeedIoctlValue(IOCTL_HID_GET_INDEXED_STRING),
    SeedIoctlValue(IOCTL_HID_GET_MANUFACTURER_STRING),
    SeedIoctlValue(IOCTL_HID_GET_POLL_FREQUENCY_MSEC),
    SeedIoctlValue(IOCTL_HID_GET_PRODUCT_STRING),
    SeedIoctlValue(IOCTL_HID_GET_SERIALNUMBER_STRING),
    SeedIoctlValue(IOCTL_HID_SET_DRIVER_CONFIG),
    SeedIoctlValue(IOCTL_HID_SET_FEATURE),
    SeedIoctlValue(IOCTL_HID_SET_POLL_FREQUENCY_MSEC),
    SeedIoctlValue(IOCTL_IDE_PASS_THROUGH),
    SeedIoctlValue(IOCTL_IEEE1284_GET_MODE),
    SeedIoctlValue(IOCTL_IEEE1284_NEGOTIATE),
    SeedIoctlValue(IOCTL_IMAPIDRV_CLOSE),
    SeedIoctlValue(IOCTL_IMAPIDRV_INFO),
    SeedIoctlValue(IOCTL_IMAPIDRV_INIT),
    SeedIoctlValue(IOCTL_IMAPIDRV_OPENEXCLUSIVE),
    SeedIoctlValue(IOCTL_INDEX),
    SeedIoctlValue(IOCTL_INTERNAL_SERENUM_REMOVE_SELF),
    SeedIoctlValue(IOCTL_INTERNAL_USB_CYCLE_PORT),
    SeedIoctlValue(IOCTL_INTERNAL_USB_ENABLE_PORT),
    SeedIoctlValue(IOCTL_INTERNAL_USB_GET_BUS_INFO),
    SeedIoctlValue(IOCTL_INTERNAL_USB_GET_BUSGUID_INFO),
    SeedIoctlValue(IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME),
    SeedIoctlValue(IOCTL_INTERNAL_USB_GET_HUB_COUNT),
    SeedIoctlValue(IOCTL_INTERNAL_USB_GET_HUB_NAME),
    SeedIoctlValue(IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO),
    SeedIoctlValue(IOCTL_INTERNAL_USB_GET_PORT_STATUS),
    SeedIoctlValue(IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO),
    SeedIoctlValue(IOCTL_INTERNAL_USB_RESET_PORT),
    SeedIoctlValue(IOCTL_INTERNAL_USB_SUBMIT_URB),
    SeedIoctlValue(IOCTL_JOY_GET_JOYREGHWCONFIG),
    SeedIoctlValue(IOCTL_JOY_GET_STATISTICS),
    SeedIoctlValue(IOCTL_KEYBOARD_INSERT_DATA),
    SeedIoctlValue(IOCTL_KEYBOARD_QUERY_ATTRIBUTES),
    SeedIoctlValue(IOCTL_KEYBOARD_QUERY_IME_STATUS),
    SeedIoctlValue(IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION),
    SeedIoctlValue(IOCTL_KEYBOARD_QUERY_INDICATORS),
    SeedIoctlValue(IOCTL_KEYBOARD_QUERY_TYPEMATIC),
    SeedIoctlValue(IOCTL_KEYBOARD_SET_IME_STATUS),
    SeedIoctlValue(IOCTL_KEYBOARD_SET_INDICATORS),
    SeedIoctlValue(IOCTL_KEYBOARD_SET_TYPEMATIC),
    SeedIoctlValue(IOCTL_KSEC_CONNECT_LSA),
    SeedIoctlValue(IOCTL_KSEC_RNG),
    SeedIoctlValue(IOCTL_KSEC_RNG_REKEY),
    SeedIoctlValue(IOCTL_LMDR_ADD_NAME),
    SeedIoctlValue(IOCTL_LMDR_ADD_NAME_DOM),
    SeedIoctlValue(IOCTL_LMDR_BECOME_BACKUP),
    SeedIoctlValue(IOCTL_LMDR_BECOME_MASTER),
    SeedIoctlValue(IOCTL_LMDR_BIND_TO_TRANSPORT),
    SeedIoctlValue(IOCTL_LMDR_BIND_TO_TRANSPORT_DOM),
    SeedIoctlValue(IOCTL_LMDR_BROWSER_PNP_ENABLE),
    SeedIoctlValue(IOCTL_LMDR_BROWSER_PNP_READ),
    SeedIoctlValue(IOCTL_LMDR_CHANGE_ROLE),
    SeedIoctlValue(IOCTL_LMDR_DEBUG_CALL),
    SeedIoctlValue(IOCTL_LMDR_DELETE_NAME),
    SeedIoctlValue(IOCTL_LMDR_DELETE_NAME_DOM),
    SeedIoctlValue(IOCTL_LMDR_ENABLE_DISABLE_TRANSPORT),
    SeedIoctlValue(IOCTL_LMDR_ENUMERATE_NAMES),
    SeedIoctlValue(IOCTL_LMDR_ENUMERATE_SERVERS),
    SeedIoctlValue(IOCTL_LMDR_ENUMERATE_TRANSPORTS),
    SeedIoctlValue(IOCTL_LMDR_GET_BROWSER_SERVER_LIST),
    SeedIoctlValue(IOCTL_LMDR_GET_MASTER_NAME),
    SeedIoctlValue(IOCTL_LMDR_IP_ADDRESS_CHANGED),
    SeedIoctlValue(IOCTL_LMDR_NETLOGON_MAILSLOT_ENABLE),
    SeedIoctlValue(IOCTL_LMDR_NETLOGON_MAILSLOT_READ),
    SeedIoctlValue(IOCTL_LMDR_NEW_MASTER_NAME),
    SeedIoctlValue(IOCTL_LMDR_QUERY_STATISTICS),
    SeedIoctlValue(IOCTL_LMDR_RENAME_DOMAIN),
    SeedIoctlValue(IOCTL_LMDR_RESET_STATISTICS),
    SeedIoctlValue(IOCTL_LMDR_START),
    SeedIoctlValue(IOCTL_LMDR_STOP),
    SeedIoctlValue(IOCTL_LMDR_UNBIND_FROM_TRANSPORT),
    SeedIoctlValue(IOCTL_LMDR_UNBIND_FROM_TRANSPORT_DOM),
    SeedIoctlValue(IOCTL_LMDR_UPDATE_STATUS),
    SeedIoctlValue(IOCTL_LMDR_WAIT_FOR_MASTER_ANNOUNCE),
    SeedIoctlValue(IOCTL_LMDR_WRITE_MAILSLOT),
    SeedIoctlValue(IOCTL_MODEM_CHECK_FOR_MODEM),
    SeedIoctlValue(IOCTL_MODEM_GET_DLE),
    SeedIoctlValue(IOCTL_MODEM_GET_MESSAGE),
    SeedIoctlValue(IOCTL_MODEM_GET_PASSTHROUGH),
    SeedIoctlValue(IOCTL_MODEM_SEND_GET_MESSAGE),
    SeedIoctlValue(IOCTL_MODEM_SEND_LOOPBACK_MESSAGE),
    SeedIoctlValue(IOCTL_MODEM_SEND_MESSAGE),
    SeedIoctlValue(IOCTL_MODEM_SET_DLE_MONITORING),
    SeedIoctlValue(IOCTL_MODEM_SET_DLE_SHIELDING),
    SeedIoctlValue(IOCTL_MODEM_SET_MIN_POWER),
    SeedIoctlValue(IOCTL_MODEM_SET_PASSTHROUGH),
    SeedIoctlValue(IOCTL_MODEM_STOP_WAVE_RECEIVE),
    SeedIoctlValue(IOCTL_MODEM_WATCH_FOR_RESUME),
    SeedIoctlValue(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME),
    SeedIoctlValue(IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS),
    SeedIoctlValue(IOCTL_MOUNTMGR_CHANGE_NOTIFY),
    SeedIoctlValue(IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES),
    SeedIoctlValue(IOCTL_MOUNTMGR_CREATE_POINT),
    SeedIoctlValue(IOCTL_MOUNTMGR_DELETE_POINTS),
    SeedIoctlValue(IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY),
    SeedIoctlValue(IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE),
    SeedIoctlValue(IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER),
    SeedIoctlValue(IOCTL_MOUNTMGR_QUERY_POINTS),
    SeedIoctlValue(IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION),
    SeedIoctlValue(IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED),
    SeedIoctlValue(IOCTL_MOUSE_INSERT_DATA),
    SeedIoctlValue(IOCTL_MOUSE_QUERY_ATTRIBUTES),
    SeedIoctlValue(IOCTL_NDIS_ADD_TDI_DEVICE),
    SeedIoctlValue(IOCTL_NDIS_DO_PNP_OPERATION),
    SeedIoctlValue(IOCTL_NDIS_ENUMERATE_INTERFACES),
    SeedIoctlValue(IOCTL_NDIS_GET_LOG_DATA),
    SeedIoctlValue(IOCTL_NDIS_QUERY_ALL_STATS),
    SeedIoctlValue(IOCTL_NDIS_QUERY_GLOBAL_STATS),
    SeedIoctlValue(IOCTL_NDIS_QUERY_SELECTED_STATS),
    SeedIoctlValue(IOCTL_PAR_ECP_HOST_RECOVERY),
    SeedIoctlValue(IOCTL_PAR_GET_DEFAULT_MODES),
    SeedIoctlValue(IOCTL_PAR_GET_DEVICE_CAPS),
    SeedIoctlValue(IOCTL_PAR_GET_READ_ADDRESS),
    SeedIoctlValue(IOCTL_PAR_GET_WRITE_ADDRESS),
    SeedIoctlValue(IOCTL_PAR_PING),
    SeedIoctlValue(IOCTL_PAR_QUERY_DEVICE_ID),
    SeedIoctlValue(IOCTL_PAR_QUERY_DEVICE_ID_SIZE),
    SeedIoctlValue(IOCTL_PAR_QUERY_INFORMATION),
    SeedIoctlValue(IOCTL_PAR_QUERY_RAW_DEVICE_ID),
    SeedIoctlValue(IOCTL_PAR_SET_INFORMATION),
    SeedIoctlValue(IOCTL_PAR_SET_READ_ADDRESS),
    SeedIoctlValue(IOCTL_PAR_SET_WRITE_ADDRESS),
    SeedIoctlValue(IOCTL_PAR_TEST),
    SeedIoctlValue(IOCTL_QUERY_DEVICE_POWER_STATE),
    SeedIoctlValue(IOCTL_READ_REGISTERS),
    SeedIoctlValue(IOCTL_REDIR_QUERY_PATH),
    SeedIoctlValue(IOCTL_RESET_PIPE),
    SeedIoctlValue(IOCTL_SCSI_FREE_DUMP_POINTERS),
    SeedIoctlValue(IOCTL_SCSI_GET_ADDRESS),
    SeedIoctlValue(IOCTL_SCSI_GET_CAPABILITIES),
    SeedIoctlValue(IOCTL_SCSI_GET_DUMP_POINTERS),
    SeedIoctlValue(IOCTL_SCSI_GET_INQUIRY_DATA),
    SeedIoctlValue(IOCTL_SCSI_MINIPORT),
    SeedIoctlValue(IOCTL_SCSI_PASS_THROUGH),
    SeedIoctlValue(IOCTL_SCSI_PASS_THROUGH_DIRECT),
    SeedIoctlValue(IOCTL_SCSI_RESCAN_BUS),
    SeedIoctlValue(IOCTL_SCSISCAN_CMD),
    SeedIoctlValue(IOCTL_SCSISCAN_GET_INFO),
    SeedIoctlValue(IOCTL_SCSISCAN_LOCKDEVICE),
    SeedIoctlValue(IOCTL_SCSISCAN_SET_TIMEOUT),
    SeedIoctlValue(IOCTL_SCSISCAN_UNLOCKDEVICE),
    SeedIoctlValue(IOCTL_SEND_USB_REQUEST),
    SeedIoctlValue(IOCTL_SERENUM_EXPOSE_HARDWARE),
    SeedIoctlValue(IOCTL_SERENUM_GET_PORT_NAME),
    SeedIoctlValue(IOCTL_SERENUM_PORT_DESC),
    SeedIoctlValue(IOCTL_SERENUM_REMOVE_HARDWARE),
    SeedIoctlValue(IOCTL_SERIAL_CLEAR_STATS),
    SeedIoctlValue(IOCTL_SERIAL_CLR_DTR),
    SeedIoctlValue(IOCTL_SERIAL_CLR_RTS),
    SeedIoctlValue(IOCTL_SERIAL_CONFIG_SIZE),
    SeedIoctlValue(IOCTL_SERIAL_GET_BAUD_RATE),
    SeedIoctlValue(IOCTL_SERIAL_GET_CHARS),
    SeedIoctlValue(IOCTL_SERIAL_GET_COMMCONFIG),
    SeedIoctlValue(IOCTL_SERIAL_GET_COMMSTATUS),
    SeedIoctlValue(IOCTL_SERIAL_GET_DTRRTS),
    SeedIoctlValue(IOCTL_SERIAL_GET_HANDFLOW),
    SeedIoctlValue(IOCTL_SERIAL_GET_LINE_CONTROL),
    SeedIoctlValue(IOCTL_SERIAL_GET_MODEM_CONTROL),
    SeedIoctlValue(IOCTL_SERIAL_GET_MODEMSTATUS),
    SeedIoctlValue(IOCTL_SERIAL_GET_PROPERTIES),
    SeedIoctlValue(IOCTL_SERIAL_GET_STATS),
    SeedIoctlValue(IOCTL_SERIAL_GET_TIMEOUTS),
    SeedIoctlValue(IOCTL_SERIAL_GET_WAIT_MASK),
    SeedIoctlValue(IOCTL_SERIAL_IMMEDIATE_CHAR),
    SeedIoctlValue(IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS),
    SeedIoctlValue(IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE),
    SeedIoctlValue(IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE),
    SeedIoctlValue(IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS),
    SeedIoctlValue(IOCTL_SERIAL_LSRMST_INSERT),
    SeedIoctlValue(IOCTL_SERIAL_LSRMST_INSERT),
    SeedIoctlValue(IOCTL_SERIAL_PURGE),
    SeedIoctlValue(IOCTL_SERIAL_RESET_DEVICE),
    SeedIoctlValue(IOCTL_SERIAL_SET_BAUD_RATE),
    SeedIoctlValue(IOCTL_SERIAL_SET_BREAK_OFF),
    SeedIoctlValue(IOCTL_SERIAL_SET_BREAK_ON),
    SeedIoctlValue(IOCTL_SERIAL_SET_CHARS),
    SeedIoctlValue(IOCTL_SERIAL_SET_COMMCONFIG),
    SeedIoctlValue(IOCTL_SERIAL_SET_DTR),
    SeedIoctlValue(IOCTL_SERIAL_SET_FIFO_CONTROL),
    SeedIoctlValue(IOCTL_SERIAL_SET_HANDFLOW),
    SeedIoctlValue(IOCTL_SERIAL_SET_LINE_CONTROL),
    SeedIoctlValue(IOCTL_SERIAL_SET_MODEM_CONTROL),
    SeedIoctlValue(IOCTL_SERIAL_SET_QUEUE_SIZE),
    SeedIoctlValue(IOCTL_SERIAL_SET_RTS),
    SeedIoctlValue(IOCTL_SERIAL_SET_TIMEOUTS),
    SeedIoctlValue(IOCTL_SERIAL_SET_WAIT_MASK),
    SeedIoctlValue(IOCTL_SERIAL_SET_XOFF),
    SeedIoctlValue(IOCTL_SERIAL_SET_XON),
    SeedIoctlValue(IOCTL_SERIAL_WAIT_ON_MASK),
    SeedIoctlValue(IOCTL_SERIAL_XOFF_COUNTER),
    SeedIoctlValue(IOCTL_SET_DEVICE_WAKE),
    SeedIoctlValue(IOCTL_SET_DEVICE_WAKE),
    SeedIoctlValue(IOCTL_SET_NUM_DEVICE_INPUT_BUFFERS),
    SeedIoctlValue(IOCTL_SET_SERVER_STATE),
    SeedIoctlValue(IOCTL_SMARTCARD_CONFISCATE),
    SeedIoctlValue(IOCTL_SMARTCARD_EJECT),
    SeedIoctlValue(IOCTL_SMARTCARD_GET_ATTRIBUTE),
    SeedIoctlValue(IOCTL_SMARTCARD_GET_LAST_ERROR),
    SeedIoctlValue(IOCTL_SMARTCARD_GET_PERF_CNTR),
    SeedIoctlValue(IOCTL_SMARTCARD_GET_STATE),
    SeedIoctlValue(IOCTL_SMARTCARD_IS_ABSENT),
    SeedIoctlValue(IOCTL_SMARTCARD_IS_PRESENT),
    SeedIoctlValue(IOCTL_SMARTCARD_POWER),
    SeedIoctlValue(IOCTL_SMARTCARD_SET_ATTRIBUTE),
    SeedIoctlValue(IOCTL_SMARTCARD_SET_PROTOCOL),
    SeedIoctlValue(IOCTL_SMARTCARD_SWALLOW),
    SeedIoctlValue(IOCTL_SMARTCARD_TRANSMIT),
    SeedIoctlValue(IOCTL_SOCKET_INFORMATION),
    SeedIoctlValue(IOCTL_STORAGE_CHECK_VERIFY),
    SeedIoctlValue(IOCTL_STORAGE_CHECK_VERIFY2),
    SeedIoctlValue(IOCTL_STORAGE_EJECT_MEDIA),
    SeedIoctlValue(IOCTL_STORAGE_EJECTION_CONTROL),
    SeedIoctlValue(IOCTL_STORAGE_FIND_NEW_DEVICES),
    SeedIoctlValue(IOCTL_STORAGE_GET_DEVICE_NUMBER),
    SeedIoctlValue(IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER),
    SeedIoctlValue(IOCTL_STORAGE_GET_MEDIA_TYPES),
    SeedIoctlValue(IOCTL_STORAGE_GET_MEDIA_TYPES_EX),
    SeedIoctlValue(IOCTL_STORAGE_LOAD_MEDIA),
    SeedIoctlValue(IOCTL_STORAGE_LOAD_MEDIA2),
    SeedIoctlValue(IOCTL_STORAGE_MCN_CONTROL),
    SeedIoctlValue(IOCTL_STORAGE_MEDIA_REMOVAL),
    SeedIoctlValue(IOCTL_STORAGE_PREDICT_FAILURE),
    SeedIoctlValue(IOCTL_STORAGE_QUERY_PROPERTY),
    SeedIoctlValue(IOCTL_STORAGE_RELEASE),
    SeedIoctlValue(IOCTL_STORAGE_RESERVE),
    SeedIoctlValue(IOCTL_STORAGE_RESET_BUS),
    SeedIoctlValue(IOCTL_STORAGE_RESET_DEVICE),
    SeedIoctlValue(IOCTL_STORAGE_SET_READ_AHEAD),
    SeedIoctlValue(IOCTL_STREAMS_GETMSG),
    SeedIoctlValue(IOCTL_STREAMS_IOCTL),
    SeedIoctlValue(IOCTL_STREAMS_POLL),
    SeedIoctlValue(IOCTL_STREAMS_PUTMSG),
    SeedIoctlValue(IOCTL_STREAMS_TDI_TEST),
    SeedIoctlValue(IOCTL_SWENUM_GET_BUS_ID),
    SeedIoctlValue(IOCTL_SWENUM_INSTALL_INTERFACE),
    SeedIoctlValue(IOCTL_SWENUM_REMOVE_INTERFACE),
    SeedIoctlValue(IOCTL_TAPE_CREATE_PARTITION),
    SeedIoctlValue(IOCTL_TAPE_EJECT_MEDIA),
    SeedIoctlValue(IOCTL_TAPE_ERASE),
    SeedIoctlValue(IOCTL_TAPE_GET_DRIVE_PARAMS),
    SeedIoctlValue(IOCTL_TAPE_GET_MEDIA_PARAMS),
    SeedIoctlValue(IOCTL_TAPE_GET_POSITION),
    SeedIoctlValue(IOCTL_TAPE_GET_STATUS),
    SeedIoctlValue(IOCTL_TAPE_LOAD_MEDIA),
    SeedIoctlValue(IOCTL_TAPE_MEDIA_REMOVAL),
    SeedIoctlValue(IOCTL_TAPE_PREPARE),
    SeedIoctlValue(IOCTL_TAPE_RELEASE),
    SeedIoctlValue(IOCTL_TAPE_RESERVE),
    SeedIoctlValue(IOCTL_TAPE_SET_DRIVE_PARAMS),
    SeedIoctlValue(IOCTL_TAPE_SET_MEDIA_PARAMS),
    SeedIoctlValue(IOCTL_TAPE_SET_POSITION),
    SeedIoctlValue(IOCTL_TAPE_WRITE_MARKS),
    SeedIoctlValue(IOCTL_TDI_ACCEPT),
    SeedIoctlValue(IOCTL_TDI_ACTION),
    SeedIoctlValue(IOCTL_TDI_ASSOCIATE_ADDRESS),
    SeedIoctlValue(IOCTL_TDI_CONNECT),
    SeedIoctlValue(IOCTL_TDI_DISASSOCIATE_ADDRESS),
    SeedIoctlValue(IOCTL_TDI_DISCONNECT),
    SeedIoctlValue(IOCTL_TDI_LISTEN),
    SeedIoctlValue(IOCTL_TDI_QUERY_INFORMATION),
    SeedIoctlValue(IOCTL_TDI_RECEIVE),
    SeedIoctlValue(IOCTL_TDI_RECEIVE_DATAGRAM),
    SeedIoctlValue(IOCTL_TDI_SEND),
    SeedIoctlValue(IOCTL_TDI_SEND_DATAGRAM),
    SeedIoctlValue(IOCTL_TDI_SET_EVENT_HANDLER),
    SeedIoctlValue(IOCTL_TDI_SET_INFORMATION),
    SeedIoctlValue(IOCTL_TRNXT_READ),
    SeedIoctlValue(IOCTL_TRNXT_XCEIVE),
    SeedIoctlValue(IOCTL_USB_DIAG_IGNORE_HUBS_OFF),
    SeedIoctlValue(IOCTL_USB_DIAG_IGNORE_HUBS_ON),
    SeedIoctlValue(IOCTL_USB_DIAGNOSTIC_MODE_OFF),
    SeedIoctlValue(IOCTL_USB_DIAGNOSTIC_MODE_ON),
    SeedIoctlValue(IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION),
    SeedIoctlValue(IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME),
    SeedIoctlValue(IOCTL_USB_GET_NODE_CONNECTION_INFORMATION),
    SeedIoctlValue(IOCTL_USB_GET_NODE_CONNECTION_NAME),
    SeedIoctlValue(IOCTL_USB_GET_NODE_INFORMATION),
    SeedIoctlValue(IOCTL_USB_GET_ROOT_HUB_NAME),
    SeedIoctlValue(IOCTL_USB_HCD_GET_STATS_1),
    SeedIoctlValue(IOCTL_USB_HCD_GET_STATS_2),
    SeedIoctlValue(IOCTL_VIDEO_DISABLE_CURSOR),
    SeedIoctlValue(IOCTL_VIDEO_DISABLE_POINTER),
    SeedIoctlValue(IOCTL_VIDEO_DISABLE_VDM),
    SeedIoctlValue(IOCTL_VIDEO_ENABLE_CURSOR),
    SeedIoctlValue(IOCTL_VIDEO_ENABLE_POINTER),
    SeedIoctlValue(IOCTL_VIDEO_ENABLE_VDM),
    SeedIoctlValue(IOCTL_VIDEO_ENUM_MONITOR_PDO),
    SeedIoctlValue(IOCTL_VIDEO_FREE_PUBLIC_ACCESS_RANGES),
    SeedIoctlValue(IOCTL_VIDEO_GET_BANK_SELECT_CODE),
    SeedIoctlValue(IOCTL_VIDEO_GET_CHILD_STATE),
    SeedIoctlValue(IOCTL_VIDEO_GET_OUTPUT_DEVICE_POWER_STATE),
    SeedIoctlValue(IOCTL_VIDEO_GET_POWER_MANAGEMENT),
    SeedIoctlValue(IOCTL_VIDEO_HANDLE_VIDEOPARAMETERS),
    SeedIoctlValue(IOCTL_VIDEO_INIT_WIN32K_CALLBACKS),
    SeedIoctlValue(IOCTL_VIDEO_LOAD_AND_SET_FONT),
    SeedIoctlValue(IOCTL_VIDEO_MAP_VIDEO_MEMORY),
    SeedIoctlValue(IOCTL_VIDEO_MONITOR_DEVICE),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_AVAIL_MODES),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_CURRENT_MODE),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_CURSOR_ATTR),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_CURSOR_POSITION),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_POINTER_ATTR),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_POINTER_CAPABILITIES),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_POINTER_POSITION),
    SeedIoctlValue(IOCTL_VIDEO_QUERY_PUBLIC_ACCESS_RANGES),
    SeedIoctlValue(IOCTL_VIDEO_REGISTER_VDM),
    SeedIoctlValue(IOCTL_VIDEO_RESET_DEVICE),
    SeedIoctlValue(IOCTL_VIDEO_RESTORE_HARDWARE_STATE),
    SeedIoctlValue(IOCTL_VIDEO_SAVE_HARDWARE_STATE),
    SeedIoctlValue(IOCTL_VIDEO_SET_CHILD_STATE_CONFIGURATION),
    SeedIoctlValue(IOCTL_VIDEO_SET_COLOR_LUT_DATA),
    SeedIoctlValue(IOCTL_VIDEO_SET_COLOR_REGISTERS),
    SeedIoctlValue(IOCTL_VIDEO_SET_CURRENT_MODE),
    SeedIoctlValue(IOCTL_VIDEO_SET_CURSOR_ATTR),
    SeedIoctlValue(IOCTL_VIDEO_SET_CURSOR_POSITION),
    SeedIoctlValue(IOCTL_VIDEO_SET_OUTPUT_DEVICE_POWER_STATE),
    SeedIoctlValue(IOCTL_VIDEO_SET_PALETTE_REGISTERS),
    SeedIoctlValue(IOCTL_VIDEO_SET_POINTER_ATTR),
    SeedIoctlValue(IOCTL_VIDEO_SET_POINTER_POSITION),
    SeedIoctlValue(IOCTL_VIDEO_SET_POWER_MANAGEMENT),
    SeedIoctlValue(IOCTL_VIDEO_SHARE_VIDEO_MEMORY),
    SeedIoctlValue(IOCTL_VIDEO_UNMAP_VIDEO_MEMORY),
    SeedIoctlValue(IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY),
    SeedIoctlValue(IOCTL_VIDEO_VALIDATE_CHILD_STATE_CONFIGURATION),
    SeedIoctlValue(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS),
    SeedIoctlValue(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS),
    SeedIoctlValue(IOCTL_VOLUME_IS_IO_CAPABLE),
    SeedIoctlValue(IOCTL_VOLUME_IS_OFFLINE),
    SeedIoctlValue(IOCTL_VOLUME_LOGICAL_TO_PHYSICAL),
    SeedIoctlValue(IOCTL_VOLUME_OFFLINE),
    SeedIoctlValue(IOCTL_VOLUME_ONLINE),
    SeedIoctlValue(IOCTL_VOLUME_PHYSICAL_TO_LOGICAL),
    SeedIoctlValue(IOCTL_VOLUME_QUERY_FAILOVER_SET),
    SeedIoctlValue(IOCTL_VOLUME_QUERY_VOLUME_NUMBER),
    SeedIoctlValue(IOCTL_VOLUME_SUPPORTS_ONLINE_OFFLINE),
    SeedIoctlValue(IOCTL_WAIT_ON_DEVICE_EVENT),
    SeedIoctlValue(IOCTL_WMI_CHECK_ACCESS),
    SeedIoctlValue(IOCTL_WMI_DISABLE_COLLECTION),
    SeedIoctlValue(IOCTL_WMI_DISABLE_EVENT),
    SeedIoctlValue(IOCTL_WMI_ENABLE_COLLECTION),
    SeedIoctlValue(IOCTL_WMI_ENABLE_EVENT),
    SeedIoctlValue(IOCTL_WMI_EXECUTE_METHOD),
    SeedIoctlValue(IOCTL_WMI_GENERATE_EVENT),
    SeedIoctlValue(IOCTL_WMI_GET_ALL_REGISTRANT),
    SeedIoctlValue(IOCTL_WMI_GET_NEXT_REGISTRANT),
    SeedIoctlValue(IOCTL_WMI_GET_REGINFO),
    SeedIoctlValue(IOCTL_WMI_GET_VERSION),
    SeedIoctlValue(IOCTL_WMI_NOTIFY_USER),
    SeedIoctlValue(IOCTL_WMI_OPEN_GUID),
    SeedIoctlValue(IOCTL_WMI_QUERY_ALL_DATA),
    SeedIoctlValue(IOCTL_WMI_QUERY_LOGGER),
    SeedIoctlValue(IOCTL_WMI_QUERY_SINGLE_INSTANCE),
    SeedIoctlValue(IOCTL_WMI_READ_NOTIFICATIONS),
    SeedIoctlValue(IOCTL_WMI_SET_SINGLE_INSTANCE),
    SeedIoctlValue(IOCTL_WMI_SET_SINGLE_ITEM),
    SeedIoctlValue(IOCTL_WMI_START_LOGGER),
    SeedIoctlValue(IOCTL_WMI_STOP_LOGGER),
    SeedIoctlValue(IOCTL_WMI_TRACE_EVENT),
    SeedIoctlValue(IOCTL_WMI_TRACE_MESSAGE),
    SeedIoctlValue(IOCTL_WMI_TRANSLATE_FILE_HANDLE),
    SeedIoctlValue(IOCTL_WMI_UPDATE_LOGGER),
    SeedIoctlValue(IOCTL_WRITE_REGISTERS),
    {NULL, 0 }
};

