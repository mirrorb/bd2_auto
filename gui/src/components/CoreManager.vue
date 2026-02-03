<template>
  <div class="app-layout">
  <Toolbar class="topbar" data-tauri-drag-region>
    <template #start>
      <div class="brand" data-tauri-drag-region>
        <span class="brand-mark">BD2 AUTO</span>
      </div>
    </template>
    <template #end>
      <div class="window-controls" data-tauri-drag-region="false">
        <Button
          class="window-btn"
          icon="pi pi-minus"
          text
          rounded
          @click="minimizeWindow"
          aria-label="最小化"
        />
        <Button
          class="window-btn"
          icon="pi pi-window-maximize"
          text
          rounded
          @click="toggleMaximize"
          aria-label="最大化"
        />
        <Button
          class="window-btn close"
          icon="pi pi-times"
          text
          rounded
          severity="danger"
          @click="closeWindow"
          aria-label="关闭"
        />
      </div>
    </template>
  </Toolbar>

  <div class="content-wrapper">
    <ScrollPanel class="content-panel">
      <section class="layout">
      <Card class="card controls">
        <template #title>
          <span class="card-title">任务控制</span>
        </template>
        <template #content>
          <div class="field">
            <label>任务选择（可多选，按顺序执行）</label>
            <div class="task-list">
              <div class="task-item" v-for="task in tasks" :key="task.name">
                <Checkbox
                  :inputId="`task-${task.name}`"
                  :value="task.name"
                  v-model="selectedTasks"
                  @change="onTasksChanged"
                />
                <label class="task-label" :for="`task-${task.name}`">{{ task.label }}</label>
                <Tag v-if="task.queue?.must_last" value="队尾" severity="secondary" class="task-tag" />
                <Tag v-if="task.queue?.is_looping" value="循环" severity="contrast" class="task-tag" />
              </div>
              <div v-if="tasks.length === 0" class="task-empty">等待任务列表...</div>
            </div>
          </div>
          <div class="queue-tip" v-if="queuePreview.length">
            当前队列：{{ queuePreview.join(' → ') }}
          </div>
          <div class="button-row">
            <Button
              :label="executionBusy ? '执行中...' : '开始执行'"
              :loading="executionBusy"
              @click="startQueue"
            />
            <Button label="停止当前任务" severity="secondary" outlined @click="stopTask" />
            <Button label="刷新状态" severity="secondary" outlined @click="refreshStatus" />
          </div>
          <div class="helper">队列顺序和任务限制由后端配置驱动。</div>
        </template>
      </Card>

      <Card v-for="task in orderedConfigs" :key="task" class="card config">
        <template #title>
          <span class="card-title" v-if="task === 'fishing_task'">钓鱼任务配置</span>
          <span class="card-title" v-else-if="task === 'hello_task'">测试任务配置</span>
        </template>
        <template #content>
          <template v-if="task === 'fishing_task'">
            <div class="grid-2">
              <div class="field">
                <label>监控窗口名称</label>
                <InputText v-model="config.monitorName" />
              </div>
              <div class="field">
                <label>显示监控</label>
                <div class="toggle">
                  <InputSwitch id="show-monitor" v-model="config.showMonitor" />
                  <label for="show-monitor">启用</label>
                </div>
              </div>
            </div>

            <Divider align="left">ROI</Divider>
            <div class="grid-4">
              <div class="field">
                <label>RX</label>
                <InputNumber v-model="config.roi.x" :step="0.001" :minFractionDigits="3" />
              </div>
              <div class="field">
                <label>RY</label>
                <InputNumber v-model="config.roi.y" :step="0.001" :minFractionDigits="3" />
              </div>
              <div class="field">
                <label>RW</label>
                <InputNumber v-model="config.roi.w" :step="0.001" :minFractionDigits="3" />
              </div>
              <div class="field">
                <label>RH</label>
                <InputNumber v-model="config.roi.h" :step="0.001" :minFractionDigits="3" />
              </div>
            </div>

            <Divider align="left">节奏与补偿</Divider>
            <div class="grid-3">
              <div class="field">
                <label>黄色补偿</label>
                <InputNumber v-model="config.padding.yellow" />
              </div>
              <div class="field">
                <label>蓝色补偿</label>
                <InputNumber v-model="config.padding.blue" />
              </div>
              <div class="field">
                <label>命中冷却(ms)</label>
                <InputNumber v-model="config.hitCooldownMs" />
              </div>
              <div class="field">
                <label>目标保持</label>
                <InputNumber v-model="config.targetPersist" />
              </div>
              <div class="field">
                <label>冻结间隔(ms)</label>
                <InputNumber v-model="config.freezeIntervalMs" />
              </div>
            </div>
          </template>

          <template v-else-if="task === 'hello_task'">
            <div class="grid-2">
              <div class="field">
                <label>重置窗口</label>
                <div class="toggle">
                  <InputSwitch id="reset-window" v-model="helloConfig.resetWindow" />
                  <label for="reset-window">启用</label>
                </div>
              </div>
              <div class="field">
                <label>窗口宽度</label>
                <InputNumber v-model="helloConfig.windowWidth" />
              </div>
              <div class="field">
                <label>窗口高度</label>
                <InputNumber v-model="helloConfig.windowHeight" />
              </div>
              <div class="field">
                <label>等待秒数</label>
                <InputNumber v-model="helloConfig.waitSeconds" />
              </div>
            </div>
          </template>
        </template>
      </Card>

      <Card class="card status">
        <template #title>
          <span class="card-title">核心状态</span>
        </template>
        <template #content>
          <div class="status-grid">
            <div>
              <div class="label">核心连接</div>
              <div class="value">
                <Tag
                  class="status-pill"
                  :severity="isOnline ? 'success' : 'secondary'"
                  :value="isOnline ? '已连接' : '等待核心响应...'"
                />
              </div>
            </div>
            <div>
              <div class="label">当前任务</div>
              <div class="value">{{ statusSummary.name || '-' }}</div>
            </div>
            <div>
              <div class="label">状态</div>
              <div class="value">{{ statusSummary.status || '-' }}</div>
            </div>
            <div>
              <div class="label">进度</div>
              <div class="value">{{ statusSummary.progress ?? '-' }}</div>
            </div>
            <div>
              <div class="label">最近事件</div>
              <div class="value">{{ lastEventText }}</div>
            </div>
          </div>
        </template>
      </Card>

      <Card class="card logs">
        <template #title>
          <div class="logs-header">
            <span class="card-title">实时日志</span>
            <div class="logs-actions">
              <Dropdown
                v-model="logLevelFilter"
                :options="logLevelOptions"
                optionLabel="label"
                optionValue="value"
                placeholder="选择等级"
              />
              <Button label="清空" severity="secondary" outlined @click="clearLogs" />
            </div>
          </div>
        </template>
        <template #content>
          <div class="log-list">
            <div v-for="entry in filteredLogs" :key="entry.id" class="log-item" :class="entry.level">
              <div class="log-meta">
                <Tag :value="entry.level.toUpperCase()" :severity="logLevelSeverity(entry.level)" class="log-tag" />
                <span class="time">{{ entry.time }}</span>
                <span class="source">{{ entry.source }}</span>
              </div>
              <div class="log-message">{{ entry.message }}</div>
              <pre v-if="entry.detail" class="log-detail">{{ entry.detail }}</pre>
            </div>
            <div v-if="filteredLogs.length === 0" class="empty">暂无日志。</div>
          </div>
        </template>
      </Card>
      </section>
    </ScrollPanel>
  </div>
</div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue'
import { listen, UnlistenFn } from '@tauri-apps/api/event'
import { invoke } from '@tauri-apps/api/core'
import { getCurrentWindow } from '@tauri-apps/api/window'
import Button from 'primevue/button'
import Card from 'primevue/card'
import Checkbox from 'primevue/checkbox'
import Divider from 'primevue/divider'
import Dropdown from 'primevue/dropdown'
import InputNumber from 'primevue/inputnumber'
import InputSwitch from 'primevue/inputswitch'
import InputText from 'primevue/inputtext'
import Tag from 'primevue/tag'
import Toolbar from 'primevue/toolbar'
import ScrollPanel from 'primevue/scrollpanel'

type LogEntry = {
  id: number
  time: string
  level: 'info' | 'warn' | 'error'
  message: string
  detail?: string
  source: string
}

type TaskMeta = {
  name: string
  label: string
  queue?: {
    singleton?: boolean
    must_last?: boolean
    is_looping?: boolean
  }
}

const tasks = ref<TaskMeta[]>([])
const selectedTasks = ref<string[]>(['fishing_task'])
const queue = ref<string[]>([])
const queueRunning = ref(false)
const poller = ref<number | null>(null)

const config = ref({
  monitorName: 'BD2 Fisher',
  showMonitor: true,
  roi: { x: 0.395, y: 0.85, w: 0.253, h: 0.051 },
  padding: { yellow: 12, blue: -2 },
  hitCooldownMs: 600,
  targetPersist: 8,
  freezeIntervalMs: 120,
})

const helloConfig = ref({
  resetWindow: true,
  windowWidth: 1280,
  windowHeight: 720,
  waitSeconds: 0,
})

const logs = ref<LogEntry[]>([])
const logLevelFilter = ref<'all' | 'info' | 'warn' | 'error'>('error')
const logLevelOptions = [
  { label: '全部等级', value: 'all' },
  { label: '仅信息', value: 'info' },
  { label: '仅警告', value: 'warn' },
  { label: '仅错误', value: 'error' },
]

const statusSummary = ref<{ name?: string; status?: string; progress?: number | string }>({})
const lastEventAt = ref<number | null>(null)

let unlistenFns: UnlistenFn[] = []
let rpcId = 1
let logId = 1

const pendingRequests = new Map<number, string>()
const appWindow = getCurrentWindow()

const isOnline = computed(() => lastEventAt.value !== null)
const isTaskRunning = computed(() => statusSummary.value.status?.startsWith('running') ?? false)
const executionBusy = computed(() => queueRunning.value || isTaskRunning.value)
const lastEventText = computed(() => {
  if (!lastEventAt.value) return '-'
  const diff = Date.now() - lastEventAt.value
  if (diff < 2000) return '刚刚'
  if (diff < 60000) return `${Math.floor(diff / 1000)} 秒前`
  return `${Math.floor(diff / 60000)} 分钟前`
})

const logLevelSeverity = (level: LogEntry['level']) => {
  if (level === 'info') return 'info'
  if (level === 'warn') return 'warning'
  return 'danger'
}

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

const pushLog = (level: LogEntry['level'], message: string, detail: unknown, source: string) => {
  const time = new Date().toLocaleTimeString()
  logs.value.unshift({
    id: logId++,
    time,
    level,
    message,
    detail: detail ? JSON.stringify(detail, null, 2) : undefined,
    source,
  })
}

const handleRpcMessage = (payload: unknown, source: string) => {
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

    if (requestMethod === 'app/getStatus' && !queueRunning.value) {
      if (
        !statusSummary.value.name ||
        statusSummary.value.status === 'completed' ||
        statusSummary.value.status === 'failed'
      ) {
        stopPoller()
      }
      return
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

const toggleMaximize = async () => {
  try {
    const isMax = await appWindow.isMaximized()
    if (isMax) {
      await appWindow.unmaximize()
    } else {
      await appWindow.maximize()
    }
  } catch (err) {
    pushLog('error', '窗口状态切换失败', err, 'ui')
  }
}

const closeWindow = async () => {
  try {
    await appWindow.close()
  } catch (err) {
    pushLog('error', '关闭窗口失败', err, 'ui')
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
    stopPoller()
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
  if (!poller.value) {
    poller.value = window.setInterval(() => {
      refreshStatus()
      checkQueueAdvance()
    }, 1200)
  }
}

const runNextTask = async () => {
  const next = queue.value.shift()
  if (!next) {
    queueRunning.value = false
    stopPoller()
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
  if (statusSummary.value.status === 'completed' || statusSummary.value.status === 'failed') {
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
  eventHandler()
  fetchTasks()
})

onUnmounted(() => {
  unlistenFns.forEach((fn) => fn())
  unlistenFns = []
  stopPoller()
})
</script>

<style>
:global(:root) {
  --topbar-height: 56px;
}

:global(html),
:global(body),
:global(#app) {
  height: 100%;
  width: 100%;
  margin: 0;
  padding: 0;
  overflow: hidden;
}

.app-layout {
  position: relative;
  height: 100%;
  width: 100%;
  display: flex;
  flex-direction: column;
}

.topbar {
  position: relative;
  height: var(--topbar-height);
  flex-shrink: 0;
  -webkit-app-region: drag;
  z-index: 1000;
}

.content-wrapper {
  flex: 1;
  overflow: hidden;
  position: relative;
}

.content-panel {
  height: 100%;
  width: 100%;
}

:global(.content-panel .p-scrollpanel-wrapper) {
  height: 100%;
}

:global(.content-panel .p-scrollpanel-content) {
  padding: 0;
}

.layout {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
  padding: 16px;
  box-sizing: border-box;
}

.field {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.task-list {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.task-item {
  display: flex;
  align-items: center;
  gap: 10px;
}

.toggle {
  display: flex;
  align-items: center;
  gap: 10px;
}

.button-row {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.grid-2 {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.grid-3 {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}

.grid-4 {
  display: grid;
  grid-template-columns: repeat(4, minmax(0, 1fr));
  gap: 12px;
}

.status-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.logs {
  grid-column: span 2;
}

.logs-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 10px;
}

.log-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.log-item {
  padding: 8px 0;
}

@media (max-width: 960px) {
  .layout {
    grid-template-columns: 1fr;
  }

  .logs {
    grid-column: span 1;
  }

  .grid-4 {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}
</style>
