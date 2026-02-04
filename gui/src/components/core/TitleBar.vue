<template>
  <div class="titlebar" @mousedown="onMouseDown">
    <div class="titlebar-content" data-tauri-drag-region>
      <div class="titlebar-left" data-tauri-drag-region>
        <i class="pi pi-box app-icon" data-tauri-drag-region></i>
        <span class="app-title" data-tauri-drag-region>BD2 AUTO</span>
      </div>
    </div>
    <div class="titlebar-controls">
      <button class="titlebar-btn titlebar-theme" @click="emit('toggle-theme')" aria-label="切换主题">
        <i :class="isDark ? 'pi pi-sun' : 'pi pi-moon'"></i>
      </button>
      <button class="titlebar-btn titlebar-minimize" @click="emit('minimize')" aria-label="最小化">
        <i class="pi pi-minus"></i>
      </button>
      <button class="titlebar-btn titlebar-close" @click="emit('close')" aria-label="关闭">
        <i class="pi pi-times"></i>
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  isDark: boolean
}>()

const emit = defineEmits<{
  (event: 'minimize'): void
  (event: 'close'): void
  (event: 'drag', payload: MouseEvent): void
  (event: 'toggle-theme'): void
}>()

const onMouseDown = (event: MouseEvent) => {
  emit('drag', event)
}
</script>

<style scoped>
.titlebar {
  height: 40px;
  min-height: 40px;
  max-height: 40px;
  background: var(--surface-card);
  border-bottom: 1px solid var(--surface-border);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-shrink: 0;
  user-select: none;
  position: relative;
  z-index: 1000;
  -webkit-app-region: drag;
}

.titlebar-content {
  flex: 1;
  height: 100%;
  display: flex;
  align-items: center;
  cursor: move; /* 显示可拖动的光标 */
}

.titlebar-left {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 0 16px;
  height: 100%;
  cursor: move; /* 显示可拖动的光标 */
}

.app-icon {
  font-size: 16px;
  color: var(--app-accent);
  pointer-events: none;
}

.app-title {
  font-size: 13px;
  font-weight: 600;
  color: var(--text-color);
  letter-spacing: 0.5px;
  pointer-events: none;
}

.titlebar-controls {
  display: flex;
  height: 100%;
  flex-shrink: 0;
  cursor: move; /* 按钮之间的间隙也可以拖动 */
  -webkit-app-region: no-drag;
}

.titlebar-btn {
  width: 46px;
  height: 100%;
  border: none;
  background: transparent;
  color: var(--text-color);
  cursor: pointer; /* 按钮本身保持pointer光标 */
  -webkit-app-region: no-drag;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: background-color 0.15s;
  font-size: 14px;
  padding: 0;
  outline: none;
}

.titlebar-btn:hover {
  background: var(--surface-hover);
}

.titlebar-btn:active {
  background: var(--surface-200);
}

.titlebar-close:hover {
  background: #e81123;
  color: white;
}

.titlebar-close:active {
  background: #c50f1f;
  color: white;
}

.titlebar-minimize:hover {
  background: #e81123;
  color: white;
}

.titlebar-minimize:active {
  background: #c50f1f;
  color: white;
}
</style>
