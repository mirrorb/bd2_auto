<template>
  <div class="app-container">
    <TitleBar
      :isDark="theme === 'dark'"
      @minimize="minimizeWindow"
      @close="closeWindow"
      @drag="handleTitlebarMouseDown"
      @toggle-theme="toggleTheme"
    />

    <div class="main-content">
      <div class="content-inner">
        <div class="cards-grid">
          <TaskControlCard
            :tasks="tasks"
            v-model:selectedTasks="selectedTasks"
            :queuePreview="queuePreview"
            :executionBusy="executionBusy"
            @tasks-change="onTasksChanged"
            @start-queue="startQueue"
            @stop-task="stopTask"
          />

          <TaskConfigCard
            v-for="task in orderedConfigs"
            :key="task"
            :task="task"
            v-model:fishingConfig="config"
            v-model:helloConfig="helloConfig"
          />

          <StatusCard :isOnline="isOnline" :statusSummary="statusSummary" :lastEventText="lastEventText" />

          <LogsCard
            :entries="filteredLogs"
            v-model:logLevelFilter="logLevelFilter"
            :logLevelOptions="logLevelOptions"
            @clear="clearLogs"
          />
        </div>
      </div>
    </div>
  </div>
</template>
<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue'
import { listen, UnlistenFn } from '@tauri-apps/api/event'
import { invoke } from '@tauri-apps/api/core'
import { getCurrentWindow } from '@tauri-apps/api/window'
import LogsCard from './core/LogsCard.vue'
import StatusCard from './core/StatusCard.vue'
import TaskConfigCard from './core/TaskConfigCard.vue'
import TaskControlCard from './core/TaskControlCard.vue'
import TitleBar from './core/TitleBar.vue'
import type { FishingConfig, HelloConfig, LogEntry, StatusSummary, TaskMeta } from './core/types'

const tasks = ref<TaskMeta[]>([])
const selectedTasks = ref<string[]>(['fishing_task'])
const queue = ref<string[]>([])
const queueRunning = ref(false)
const poller = ref<number | null>(null)

const config = ref<FishingConfig>({
  monitorName: 'BD2 Fishing Monitor',
  showMonitor: true,
  roi: { x: 0.395, y: 0.85, w: 0.253, h: 0.051 },
  padding: { yellow: 12, blue: -2 },
  hitCooldownMs: 600,
  targetPersist: 8,
  freezeIntervalMs: 120,
})

const helloConfig = ref<HelloConfig>({
  resetWindow: true,
  windowWidth: 1280,
  windowHeight: 720,
  waitSeconds: 0,
})

const logs = ref<LogEntry[]>([])
const logLevelFilter = ref<'all' | 'info' | 'warn' | 'error'>('error')
const logLevelOptions: Array<{ label: string; value: 'all' | 'info' | 'warn' | 'error' }> = [
  { label: '全部等级', value: 'all' },
  { label: '仅信息', value: 'info' },
  { label: '仅警告', value: 'warn' },
  { label: '仅错误', value: 'error' },
]

const statusSummary = ref<StatusSummary>({})
const lastEventAt = ref<number | null>(null)
const lastStatusAt = ref<number | null>(null)
const clockTick = ref(Date.now())

let unlistenFns: UnlistenFn[] = []
let rpcId = 1
let logId = 1

const pendingRequests = new Map<number, string>()
const appWindow = getCurrentWindow()

const theme = ref<'light' | 'dark'>('light')

const applyTheme = (value: 'light' | 'dark') => {
  theme.value = value
  document.documentElement.dataset.theme = value
  localStorage.setItem('bd2-theme', value)
}

const initTheme = () => {
  const saved = localStorage.getItem('bd2-theme')
  if (saved === 'light' || saved === 'dark') {
    applyTheme(saved)
    return
  }
  applyTheme('dark')
}

const toggleTheme = () => {
  applyTheme(theme.value === 'dark' ? 'light' : 'dark')
}

const isOnline = computed(() => {
  if (lastStatusAt.value === null) return false
  return clockTick.value - lastStatusAt.value < 3000
})
const isTaskRunning = computed(() => statusSummary.value.status?.startsWith('运行中') ?? false)
const executionBusy = computed(() => queueRunning.value || isTaskRunning.value)
const lastEventText = computed(() => {
  if (!lastEventAt.value) return '-'
  const diff = clockTick.value - lastEventAt.value
  if (diff < 2000) return '刚刚'
  if (diff < 60000) return `${Math.floor(diff / 1000)} 秒前`
  return `${Math.floor(diff / 60000)} 分钟前`
})

const taskMap = computed(() => {
  const map = new Map<string, TaskMeta>()
  tasks.value.forEach((task) => map.set(task.name, task))
  return map
})

const normalizeTaskOrder = (input: string[]) => {
  const seen = new Set<string>()
  const unique: string[] = []
  for (const task of input) {
    if (seen.has(task)) continue
    seen.add(task)
    unique.push(task)
  }
  const regular: string[] = []
  const mustLast: string[] = []
  for (const task of unique) {
    const meta = taskMap.value.get(task)
    if (meta?.queue?.must_last) {
      mustLast.push(task)
    } else {
      regular.push(task)
    }
  }
  return [...regular, ...mustLast]
}

const queuePreview = computed(() =>
  normalizeTaskOrder(selectedTasks.value).map(
    (task) => taskMap.value.get(task)?.label ?? task
  )
)

const orderedConfigs = computed(() => normalizeTaskOrder(selectedTasks.value))
const filteredLogs = computed(() => {
  if (logLevelFilter.value === 'all') return logs.value
  return logs.value.filter((entry) => entry.level === logLevelFilter.value)
})

const onTasksChanged = () => {
  selectedTasks.value = normalizeTaskOrder(selectedTasks.value)
}

const formatSource = (source: string) => {
  switch (source) {
    case 'stdout':
      return '标准输出'
    case 'core-error':
      return '核心错误'
    case 'core-message':
      return '核心消息'
    case 'core-terminated':
      return '核心退出'
    case 'ui':
      return '界面'
    case 'log':
      return '日志'
    default:
      return source
  }
}

const formatLevel = (level: string) => {
  switch (level) {
    case 'info':
      return '信息'
    case 'warn':
      return '警告'
    case 'error':
      return '错误'
    default:
      return level
  }
}

const formatDetail = (detail: unknown) => {
  if (!detail) return undefined
  return JSON.stringify(
    detail,
    (key, value) => {
      if (key === 'level' && typeof value === 'string') {
        return formatLevel(value)
      }
      if (key === 'type' && typeof value === 'string') {
        return formatSource(value)
      }
      return value
    },
    2
  )
}

const pushLog = (level: LogEntry['level'], message: string, detail: unknown, source: string) => {
  const time = new Date().toLocaleTimeString()
  logs.value.unshift({
    id: logId++,
    time,
    level,
    message,
    detail: formatDetail(detail),
    source: formatSource(source),
  })
}

const handleRpcMessage = (payload: unknown, source: string) => {
  if (source === 'core-terminated') {
    lastEventAt.value = Date.now()
    lastStatusAt.value = 0
    statusSummary.value = {}
    queueRunning.value = false
    queue.value = []
    pushLog('error', '后端已退出，任务已中止。', payload, source)
    return
  }

  lastEventAt.value = Date.now()
  let data: any = payload
  if (typeof payload === 'string') {
    try {
      data = JSON.parse(payload)
    } catch {
      pushLog('info', payload, null, source)
      return
    }
  }

  if (data?.method) {
    const level = (data.params?.level ?? 'info') as LogEntry['level']
    const message = data.params?.message ?? data.method
    pushLog(level, message, data.params?.payload ?? data.params, data.method)
    return
  }

  const requestMethod = typeof data?.id === 'number' ? pendingRequests.get(data.id) : undefined
  if (typeof data?.id === 'number') {
    pendingRequests.delete(data.id)
  }

  if (data?.error) {
    pushLog('error', data.error.message ?? 'RPC 错误', data.error, source)
    return
  }

  if (data?.result) {
    if (Array.isArray(data.result.tasks)) {
      tasks.value = data.result.tasks as TaskMeta[]
      const valid = selectedTasks.value.filter((t) => taskMap.value.has(t))
      selectedTasks.value = normalizeTaskOrder(valid)
      if (selectedTasks.value.length === 0 && tasks.value.length > 0) {
        selectedTasks.value = normalizeTaskOrder([tasks.value[0].name])
      }
      return
    }
    if (data.result?.active_task) {
      statusSummary.value = {
        name: data.result.active_task.name,
        status: data.result.active_task.status,
        progress: data.result.active_task.progress,
      }
    } else {
      statusSummary.value = {}
    }

    if (requestMethod === 'app/getStatus') {
      lastStatusAt.value = Date.now()
      if (!queueRunning.value) {
        return
      }
    }
    pushLog('info', '收到响应', data.result, source)
    return
  }

  pushLog('info', '未解析的消息', data, source)
}

const sendRpc = async (method: string, params: Record<string, any>) => {
  const requestId = rpcId++
  const request = {
    jsonrpc: '2.0',
    id: requestId,
    method,
    params,
  }
  pendingRequests.set(requestId, method)
  await invoke('core_input', { input: JSON.stringify(request) })
}

const fetchTasks = async () => {
  try {
    await sendRpc('app/getTasks', {})
  } catch (err) {
    pushLog('error', '获取任务列表失败', err, 'ui')
  }
}

const minimizeWindow = async () => {
  try {
    await appWindow.minimize()
  } catch (err) {
    pushLog('error', '最小化失败', err, 'ui')
  }
}

const closeWindow = async () => {
  try {
    await appWindow.close()
  } catch (err) {
    pushLog('error', '关闭窗口失败', err, 'ui')
  }
}

const handleTitlebarMouseDown = async (event: MouseEvent) => {
  if (event.button !== 0) return
  const target = event.target as HTMLElement | null
  if (target?.closest('.titlebar-controls')) return
  try {
    await appWindow.startDragging()
  } catch (err) {
    pushLog('error', '拖动窗口失败', err, 'ui')
  }
}

const buildFishingConfig = () => ({
  monitor_name: config.value.monitorName,
  show_monitor: config.value.showMonitor,
  roi: {
    x: config.value.roi.x,
    y: config.value.roi.y,
    w: config.value.roi.w,
    h: config.value.roi.h,
  },
  padding: {
    yellow: config.value.padding.yellow,
    blue: config.value.padding.blue,
  },
  hit_cooldown_ms: config.value.hitCooldownMs,
  target_persist: config.value.targetPersist,
  freeze_interval_ms: config.value.freezeIntervalMs,
})

const startTask = async (taskName: string) => {
  const params: Record<string, any> = { task_name: taskName }
  if (taskName === 'fishing_task') {
    params.config = buildFishingConfig()
  } else if (taskName === 'hello_task') {
    params.reset_window = helloConfig.value.resetWindow
    params.window_width = helloConfig.value.windowWidth
    params.window_height = helloConfig.value.windowHeight
    params.wait_seconds = helloConfig.value.waitSeconds
  }
  await sendRpc('task/start', params)
}

const stopTask = async () => {
  try {
    await sendRpc('task/stop', {})
    queueRunning.value = false
    queue.value = []
  } catch (err) {
    pushLog('error', '发送停止请求失败', err, 'ui')
  }
}

const refreshStatus = async () => {
  try {
    await sendRpc('app/getStatus', {})
  } catch (err) {
    pushLog('error', '获取状态失败', err, 'ui')
  }
}

const clearLogs = () => {
  logs.value = []
}

const stopPoller = () => {
  if (poller.value) {
    window.clearInterval(poller.value)
    poller.value = null
  }
}

const startQueue = async () => {
  if (queueRunning.value || isTaskRunning.value) return
  if (selectedTasks.value.length === 0) {
    pushLog('warn', '请先选择至少一个任务', null, 'ui')
    return
  }
  const normalized = normalizeTaskOrder(selectedTasks.value)
  selectedTasks.value = normalized
  queue.value = [...normalized]
  queueRunning.value = true
  await runNextTask()
}

const runNextTask = async () => {
  const next = queue.value.shift()
  if (!next) {
    queueRunning.value = false
    pushLog('info', '任务队列执行完成', null, 'ui')
    return
  }
  try {
    await startTask(next)
    pushLog('info', `开始执行任务：${next}`, null, 'ui')
  } catch (err) {
    pushLog('error', `启动任务失败：${next}`, err, 'ui')
    await runNextTask()
  }
}

const checkQueueAdvance = () => {
  if (!queueRunning.value) return
  if (!statusSummary.value.name) {
    runNextTask()
    return
  }
  if (statusSummary.value.status === '已完成' || statusSummary.value.status === '失败') {
    runNextTask()
  }
}

const eventHandler = async () => {
  unlistenFns.push(await listen('core-output', (event) => handleRpcMessage(event.payload, 'stdout')))
  unlistenFns.push(await listen('core-error', (event) => handleRpcMessage(event.payload, 'core-error')))
  unlistenFns.push(await listen('core-message', (event) => handleRpcMessage(event.payload, 'core-message')))
  unlistenFns.push(await listen('core-terminated', (event) => handleRpcMessage(event.payload, 'core-terminated')))
}

onMounted(() => {
  initTheme()
  eventHandler()
  fetchTasks()
  if (!poller.value) {
    poller.value = window.setInterval(() => {
      clockTick.value = Date.now()
      refreshStatus()
      checkQueueAdvance()
    }, 1000)
  }
})

onUnmounted(() => {
  unlistenFns.forEach((fn) => fn())
  unlistenFns = []
  stopPoller()
})
</script>

<style scoped>
/* 主题背景变量（前端决定浅色/深色） */
:global(:root) {
  --app-bg: #e9edf2;
  --app-surface: #ffffff;
  --app-border: rgba(0, 0, 0, 0.26);
  --app-accent: #2563eb;
  --app-accent-contrast: #ffffff;
  --control-bg: #ffffff;
  --control-border: rgba(0, 0, 0, 0.16);
  --control-text: #111827;
  --control-hover: #f3f4f6;
  --tag-secondary-bg: #eef1f5;
  --tag-secondary-text: #4b5563;
  --tag-contrast-bg: #111827;
  --tag-contrast-text: #ffffff;
  --surface-ground: var(--app-bg);
  --surface-card: var(--app-surface);
  --surface-border: var(--app-border);
  --surface-50: #ffffff;
  --surface-100: #f7f7f9;
  --surface-200: #eef1f5;
  --surface-400: #cbd5e1;
  --surface-500: #94a3b8;
  --surface-hover: rgba(0, 0, 0, 0.04);
  --text-color: #111827;
  --text-color-secondary: #6b7280;
}

:global(:root[data-theme='dark']) {
  --app-bg: #0b0d11;
  --app-surface: #171d24;
  --app-border: rgba(255, 255, 255, 0.14);
  --app-accent: #60a5fa;
  --app-accent-contrast: #0b1220;
  --control-bg: #1c2330;
  --control-border: rgba(255, 255, 255, 0.14);
  --control-text: #e5e7eb;
  --control-hover: #2a3545;
  --tag-secondary-bg: #1f2937;
  --tag-secondary-text: #e2e8f0;
  --tag-contrast-bg: var(--app-accent);
  --tag-contrast-text: var(--app-accent-contrast);
  --card-title-color: #f8fafc;
  --card-title-muted: #d1d5db;
  --surface-ground: var(--app-bg);
  --surface-card: var(--app-surface);
  --surface-border: var(--app-border);
  --surface-50: #1b212a;
  --surface-100: #202834;
  --surface-200: #2a3442;
  --surface-400: #3b495b;
  --surface-500: #4e6076;
  --surface-hover: rgba(255, 255, 255, 0.06);
  --text-color: #e5e7eb;
  --text-color-secondary: #cbd5e1;
}

@media (prefers-color-scheme: dark) {
  :global(:root:not([data-theme])) {
    --app-bg: #0b0d11;
    --app-surface: #171d24;
    --app-border: rgba(255, 255, 255, 0.14);
    --app-accent: #60a5fa;
    --app-accent-contrast: #0b1220;
    --control-bg: #1c2330;
    --control-border: rgba(255, 255, 255, 0.14);
    --control-text: #e5e7eb;
    --control-hover: #2a3545;
    --tag-secondary-bg: #1f2937;
    --tag-secondary-text: #e2e8f0;
    --tag-contrast-bg: #2563eb;
    --tag-contrast-text: #ffffff;
    --surface-ground: var(--app-bg);
    --surface-card: var(--app-surface);
    --surface-border: var(--app-border);
    --surface-50: #1b212a;
    --surface-100: #202834;
    --surface-200: #2a3442;
    --surface-400: #3b495b;
    --surface-500: #4e6076;
    --surface-hover: rgba(255, 255, 255, 0.06);
    --text-color: #e5e7eb;
    --text-color-secondary: #cbd5e1;
  }
}

:global(html),
:global(body),
:global(#app) {
  width: 100%;
  height: 100%;
  background: var(--app-bg);
  overflow: hidden;
}

/* 全局重置 */
:deep(*) {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

/* 应用容器 - 圆角内容壳体 */
.app-container {
  width: 100vw;
  height: 100vh;
  max-width: 100vw;
  max-height: 100vh;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  background: var(--app-surface);
  position: fixed;
  inset: 0;
  border-radius: 0;
  border: 1px solid var(--app-border);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.35);
}

:global(.p-card) {
  background: var(--surface-card);
  border: 1px solid var(--surface-border);
  box-shadow: 0 1px 0 rgba(15, 23, 42, 0.05), 0 8px 18px rgba(15, 23, 42, 0.08);
}

:global(:root[data-theme='dark'] .p-card) {
  background: var(--surface-card);
  border: 1px solid var(--surface-border);
  box-shadow: 0 1px 0 rgba(255, 255, 255, 0.04), 0 10px 22px rgba(0, 0, 0, 0.35);
}

:global(:root[data-theme='dark'] .card-title) {
  color: var(--card-title-color) !important;
}

:global(:root[data-theme='dark'] .status-label) {
  color: var(--card-title-muted) !important;
}

:global(:root[data-theme='dark'] .p-inputtext),
:global(:root[data-theme='dark'] .p-inputnumber-input),
:global(:root[data-theme='dark'] .p-dropdown) {
  background: var(--control-bg) !important;
  border-color: var(--control-border) !important;
  color: var(--control-text) !important;
}

:global(:root[data-theme='dark'] .p-dropdown-label),
:global(:root[data-theme='dark'] .p-dropdown-trigger) {
  background: var(--control-bg) !important;
  color: var(--control-text) !important;
}

:global(:root[data-theme='dark'] .p-dropdown) {
  background: var(--control-bg) !important;
  border-color: var(--control-border) !important;
}

:global(:root[data-theme='dark'] .p-dropdown-panel) {
  background: var(--control-bg) !important;
  border-color: var(--control-border) !important;
}

:global(:root[data-theme='dark'] .p-dropdown-item) {
  color: var(--text-color);
}

:global(:root[data-theme='dark'] .p-dropdown-item:not(.p-highlight):hover) {
  background: var(--control-hover) !important;
}

:global(:root[data-theme='dark'] .p-dropdown-item.p-highlight) {
  background: var(--surface-200) !important;
}

:global(:root[data-theme='dark'] .p-select) {
  background: var(--control-bg) !important;
  border-color: var(--control-border) !important;
  color: var(--control-text) !important;
}

:global(:root[data-theme='dark'] .p-select-label),
:global(:root[data-theme='dark'] .p-select-dropdown),
:global(:root[data-theme='dark'] .p-select-dropdown-icon),
:global(:root[data-theme='dark'] .p-select-clear-icon) {
  background: var(--control-bg) !important;
  color: var(--control-text) !important;
}

:global(:root[data-theme='dark'] .p-select-overlay) {
  background: var(--control-bg) !important;
  border-color: var(--control-border) !important;
}

:global(:root[data-theme='dark'] .p-select-option) {
  color: var(--text-color) !important;
}

:global(:root[data-theme='dark'] .p-select-option:not(.p-select-option-selected):hover),
:global(:root[data-theme='dark'] .p-select-option.p-focus:not(.p-highlight)) {
  background: var(--control-hover) !important;
}

:global(:root[data-theme='dark'] .p-select-option.p-select-option-selected),
:global(:root[data-theme='dark'] .p-select-option.p-highlight),
:global(:root[data-theme='dark'] .p-select-option[aria-selected='true']) {
  background: var(--surface-200) !important;
  color: var(--text-color) !important;
}

:global(:root[data-theme='dark'] .p-dropdown-item) {
  color: var(--text-color) !important;
}

:global(:root[data-theme='dark'] .p-dropdown-item:not(.p-highlight):hover),
:global(:root[data-theme='dark'] .p-dropdown-item.p-focus:not(.p-highlight)) {
  background: var(--control-hover) !important;
}

:global(:root[data-theme='dark'] .p-dropdown-item.p-highlight),
:global(:root[data-theme='dark'] .p-dropdown-item[aria-selected='true']) {
  background: var(--surface-200) !important;
  color: var(--text-color) !important;
}

:global(:root[data-theme='dark'] .p-button.p-button-outlined) {
  border-color: var(--control-border) !important;
  color: var(--text-color) !important;
}

:global(:root[data-theme='dark'] .p-button.p-button-outlined:hover) {
  background: var(--control-hover) !important;
}

:global(:root[data-theme='dark'] .p-button.p-button-primary) {
  color: var(--app-accent-contrast) !important;
}

:global(:root[data-theme='dark'] .p-button.p-button-primary .p-button-label),
:global(:root[data-theme='dark'] .p-button.p-button-primary .p-button-icon) {
  color: var(--app-accent-contrast) !important;
}

:global(:root[data-theme='dark'] .p-button:not(.p-button-outlined):not(.p-button-text):not(.p-button-link)) {
  color: var(--app-accent-contrast) !important;
}

:global(:root[data-theme='dark'] .p-button:not(.p-button-outlined):not(.p-button-text):not(.p-button-link) .p-button-label),
:global(:root[data-theme='dark'] .p-button:not(.p-button-outlined):not(.p-button-text):not(.p-button-link) .p-button-icon) {
  color: var(--app-accent-contrast) !important;
}

:global(:root[data-theme='dark'] .p-tag.p-tag-secondary),
:global(:root[data-theme='dark'] .p-tag-secondary) {
  background: var(--tag-secondary-bg) !important;
  color: var(--tag-secondary-text) !important;
}

:global(:root[data-theme='dark'] .p-tag.p-tag-contrast),
:global(:root[data-theme='dark'] .p-tag-contrast) {
  background: var(--tag-contrast-bg) !important;
  color: var(--tag-contrast-text) !important;
}

:global(.p-tag.p-tag-success),
:global(.p-tag-success) {
  background: var(--app-accent) !important;
  border-color: var(--app-accent) !important;
  color: var(--app-accent-contrast) !important;
}

:global(:root[data-theme='dark'] .p-checkbox .p-checkbox-box) {
  background: var(--control-bg) !important;
  border-color: var(--control-border) !important;
}

:global(:root[data-theme='dark'] .p-checkbox:not(.p-disabled) .p-checkbox-box:hover) {
  border-color: var(--app-accent) !important;
}

:global(:root[data-theme='dark'] .p-checkbox-checked .p-checkbox-box) {
  background: var(--app-accent) !important;
  border-color: var(--app-accent) !important;
}

:global(:root[data-theme='dark'] .p-checkbox-checked .p-checkbox-icon) {
  color: var(--app-accent-contrast) !important;
}

:global(:root[data-theme='dark'] .p-checkbox-checked:not(.p-disabled):has(.p-checkbox-input:hover) .p-checkbox-box) {
  background: var(--app-accent) !important;
  border-color: var(--app-accent) !important;
}

:global(:root[data-theme='dark'] .p-checkbox .p-checkbox-box.p-highlight) {
  background: var(--app-accent) !important;
  border-color: var(--app-accent) !important;
}

:global(:root[data-theme='dark'] .p-checkbox .p-checkbox-box.p-highlight .p-checkbox-icon) {
  color: var(--app-accent-contrast) !important;
}

:global(:root[data-theme='dark'] .p-toggleswitch-slider) {
  background: var(--control-bg) !important;
  border-color: var(--control-border) !important;
}

:global(:root[data-theme='dark'] .p-toggleswitch-handle) {
  background: var(--text-color) !important;
  color: var(--app-accent-contrast) !important;
}

:global(:root[data-theme='dark'] .p-toggleswitch.p-toggleswitch-checked .p-toggleswitch-slider) {
  background: var(--app-accent) !important;
  border-color: var(--app-accent) !important;
}

:global(:root[data-theme='dark'] .p-toggleswitch.p-toggleswitch-checked .p-toggleswitch-handle) {
  background: var(--app-accent-contrast) !important;
  color: var(--app-accent) !important;
}

:global(:root[data-theme='dark'] .p-toggleswitch:not(.p-disabled):has(.p-toggleswitch-input:hover) .p-toggleswitch-slider) {
  background: var(--control-hover) !important;
  border-color: var(--control-border) !important;
}

:global(:root[data-theme='dark'] .p-toggleswitch:not(.p-disabled):has(.p-toggleswitch-input:hover) .p-toggleswitch-handle) {
  background: var(--text-color) !important;
}

:global(:root[data-theme='dark'] .p-toggleswitch:not(.p-disabled):has(.p-toggleswitch-input:hover).p-toggleswitch-checked .p-toggleswitch-slider) {
  background: var(--app-accent) !important;
  border-color: var(--app-accent) !important;
}

/* ==================== 主内容区 - 可滚动 ==================== */
.main-content {
  flex: 1;
  overflow-y: scroll; /* 改为 scroll 强制显示滚动条 */
  overflow-x: hidden;
  position: relative;
  min-height: 0;
  width: 100%;
}

/* 自定义原生滚动条样式 (Webkit) */
.main-content::-webkit-scrollbar {
  width: 10px;
}

.main-content::-webkit-scrollbar-track {
  background: var(--surface-100);
  border-radius: 5px;
  margin: 4px 0;
}

.main-content::-webkit-scrollbar-thumb {
  background: var(--surface-400);
  border-radius: 5px;
  border: 2px solid var(--surface-100);
}

.main-content::-webkit-scrollbar-thumb:hover {
  background: var(--surface-500);
}

/* Firefox 滚动条样式 */
.main-content {
  scrollbar-width: thin;
  scrollbar-color: var(--surface-400) var(--surface-100);
}

.content-inner {
  padding: 16px;
}

.cards-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
  min-width: 0;
}

/* ==================== 卡片样式 ==================== */
.logs-card {
  grid-column: 1 / -1;
  min-width: 0;
}

.control-card,
.config-card,
.status-card {
  min-width: 0;
  overflow: hidden;
}

:deep(.card-title) {
  font-size: 1.1rem;
  font-weight: 600;
  color: var(--text-color);
}

/* 确保Card内容不溢出 */
:deep(.p-card-body),
:deep(.p-card-content) {
  min-width: 0;
  overflow: hidden;
}

/* ==================== 响应式 ==================== */
@media (max-width: 960px) {
  .cards-grid {
    grid-template-columns: 1fr;
  }

  .logs-card {
    grid-column: 1;
  }
}
</style>



