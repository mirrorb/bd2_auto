export type LogEntry = {
  id: number
  time: string
  level: 'info' | 'warn' | 'error'
  message: string
  detail?: string
  source: string
}

export type TaskMeta = {
  name: string
  label: string
  queue?: {
    singleton?: boolean
    must_last?: boolean
    is_looping?: boolean
  }
}

export type StatusSummary = {
  name?: string
  status?: string
  progress?: number | string
}

export type FishingConfig = {
  monitorName: string
  showMonitor: boolean
  roi: {
    x: number
    y: number
    w: number
    h: number
  }
  padding: {
    yellow: number
    blue: number
  }
  hitCooldownMs: number
  targetPersist: number
  freezeIntervalMs: number
}

export type HelloConfig = {
  resetWindow: boolean
  windowWidth: number
  windowHeight: number
  waitSeconds: number
}
