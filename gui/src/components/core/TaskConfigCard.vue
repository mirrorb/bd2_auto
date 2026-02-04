<template>
  <Card class="config-card">
    <template #title>
      <span class="card-title" v-if="task === 'fishing_task'">钓鱼任务配置</span>
      <span class="card-title" v-else-if="task === 'hello_task'">测试任务配置</span>
    </template>
    <template #content>
      <template v-if="task === 'fishing_task'">
        <div class="grid-2">
          <div class="field">
            <label>监控窗口名称</label>
            <InputText v-model="fishingConfig.monitorName" />
          </div>
          <div class="field">
            <label>显示监控</label>
            <div class="toggle">
              <ToggleSwitch :id="`show-monitor-${task}`" v-model="fishingConfig.showMonitor" />
              <label :for="`show-monitor-${task}`">启用</label>
            </div>
          </div>
        </div>

        <Divider align="left" class="section-divider">ROI</Divider>
        <div class="grid-4">
          <div class="field">
            <label>RX</label>
            <InputNumber v-model="fishingConfig.roi.x" :step="0.001" :minFractionDigits="3" />
          </div>
          <div class="field">
            <label>RY</label>
            <InputNumber v-model="fishingConfig.roi.y" :step="0.001" :minFractionDigits="3" />
          </div>
          <div class="field">
            <label>RW</label>
            <InputNumber v-model="fishingConfig.roi.w" :step="0.001" :minFractionDigits="3" />
          </div>
          <div class="field">
            <label>RH</label>
            <InputNumber v-model="fishingConfig.roi.h" :step="0.001" :minFractionDigits="3" />
          </div>
        </div>

        <Divider align="left" class="section-divider">节奏与补偿</Divider>
        <div class="grid-3">
          <div class="field">
            <label>黄色补偿</label>
            <InputNumber v-model="fishingConfig.padding.yellow" />
          </div>
          <div class="field">
            <label>蓝色补偿</label>
            <InputNumber v-model="fishingConfig.padding.blue" />
          </div>
          <div class="field">
            <label>命中冷却(ms)</label>
            <InputNumber v-model="fishingConfig.hitCooldownMs" />
          </div>
          <div class="field">
            <label>目标保持</label>
            <InputNumber v-model="fishingConfig.targetPersist" />
          </div>
          <div class="field">
            <label>冻结间隔(ms)</label>
            <InputNumber v-model="fishingConfig.freezeIntervalMs" />
          </div>
        </div>
      </template>

      <template v-else-if="task === 'hello_task'">
        <div class="grid-2">
          <div class="field">
            <label>重置窗口</label>
            <div class="toggle">
              <ToggleSwitch :id="`reset-window-${task}`" v-model="helloConfig.resetWindow" />
              <label :for="`reset-window-${task}`">启用</label>
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
</template>

<script setup lang="ts">
import Card from 'primevue/card'
import Divider from 'primevue/divider'
import InputNumber from 'primevue/inputnumber'
import ToggleSwitch from 'primevue/toggleswitch'
import InputText from 'primevue/inputtext'
import type { FishingConfig, HelloConfig } from './types'

defineProps<{
  task: string
}>()

const fishingConfig = defineModel<FishingConfig>('fishingConfig', { required: true })
const helloConfig = defineModel<HelloConfig>('helloConfig', { required: true })
</script>

<style scoped>
.field {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.field label {
  font-size: 0.875rem;
  font-weight: 500;
  color: var(--text-color-secondary);
}

.toggle {
  display: flex;
  align-items: center;
  gap: 10px;
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

.field :deep(.p-inputtext),
.field :deep(.p-inputnumber-input) {
  min-width: 0;
  width: 100%;
}

.field :deep(.p-inputnumber) {
  width: 100%;
}

.section-divider :deep(.p-divider-content) {
  font-size: 1rem;
  font-weight: 700;
  color: var(--text-color);
  letter-spacing: 0.5px;
}

.section-divider :deep(.p-divider-content) {
  background: var(--surface-card);
  padding: 0 8px;
}

@media (max-width: 1200px) {
  .grid-4 {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 960px) {
  .grid-3 {
    grid-template-columns: repeat(2, minmax(0, 1fr));
  }
}

@media (max-width: 600px) {
  .grid-2,
  .grid-3,
  .grid-4 {
    grid-template-columns: 1fr;
  }
}
</style>
