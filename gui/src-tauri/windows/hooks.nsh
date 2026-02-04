!macro NSIS_HOOK_PREINSTALL
  ; Clean old core dir on install/upgrade to avoid stale files.
  SetDetailsView show
  DetailPrint "清理旧 core 目录: $INSTDIR\\core"
  IfFileExists "$INSTDIR\\core\\*.*" 0 +2
  RMDir /r "$INSTDIR\\core"
  IfFileExists "$INSTDIR\\core\\*.*" 0 +2
  DetailPrint "警告: core 目录仍存在，可能被占用。"
!macroend

!macro NSIS_HOOK_PREUNINSTALL
  ; Ensure core dir removed on uninstall.
  SetDetailsView show
  DetailPrint "卸载时清理 core 目录: $INSTDIR\\core"
  IfFileExists "$INSTDIR\\core\\*.*" 0 +2
  RMDir /r "$INSTDIR\\core"
  IfFileExists "$INSTDIR\\core\\*.*" 0 +2
  DetailPrint "警告: core 目录仍存在，可能被占用。"
!macroend
