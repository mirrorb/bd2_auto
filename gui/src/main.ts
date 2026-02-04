import { createApp } from "vue";
import PrimeVue from "primevue/config";
import { definePreset, palette } from "@primeuix/themes";
import Aura from "@primeuix/themes/aura";
import "primeicons/primeicons.css";
import App from "./App.vue";

const app = createApp(App);
const BlueAura = definePreset(Aura, {
  semantic: {
    primary: palette("{blue}"),
  },
});
app.use(PrimeVue, {
  theme: {
    preset: BlueAura,
  },
});
app.mount("#app");
