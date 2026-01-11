<template>
    <div class="row gy-3 mt-0">
        <div class="col-sm-3 col-md-2" :style="[batteryData.length == 1 ? { display: 'none' } : {}]">
            <div
                class="nav nav-pills row-cols-sm-1 gap-3"
                id="v-pills-battery-tab"
                role="tablist"
                aria-orientation="vertical"
            >
                <button
                    v-for="battery in batteryData"
                    :key="battery.uid"
                    class="nav-link border border-primary text-break"
                    :id="'v-pills-battery-' + battery.uid + '-tab'"
                    data-bs-toggle="pill"
                    :data-bs-target="'#v-pills-battery-' + battery.uid"
                    type="button"
                    role="tab"
                    aria-controls="'v-pills-battery-' + battery.uid"
                    aria-selected="true"
                >
                    <div class="d-flex align-items-center">
                        <div class="me-2" style="padding-right: 4px">
                            <BIconBatteryCharging
                                v-if="getValue(battery, 'current', 0) > 0"
                                style="font-size: 24px"
                                v-tooltip
                                :title="
                                    $t('home.battery.charging', {
                                        soc: $n(getValue(battery, 'SoC', 100), 'decimalNoDigits'),
                                    })
                                "
                            />
                            <BIconBatteryFull
                                v-else-if="getValue(battery, 'SoC', 100) == 100"
                                style="font-size: 24px; color: green"
                                v-tooltip
                                :title="$t('home.battery.full')"
                            />
                            <BIconBatteryFull
                                v-else-if="
                                    getValue(battery, 'SoC', 100) >= getValue(battery, 'maxSoC', 100, 'settings')
                                "
                                style="font-size: 24px"
                                v-tooltip
                                :title="
                                    $t('home.battery.high', {
                                        soc: $n(getValue(battery, 'SoC', 100), 'decimalNoDigits'),
                                    })
                                "
                            />
                            <BIconBattery
                                v-else-if="getValue(battery, 'SoC', 100) <= getValue(battery, 'minSoC', 0, 'settings')"
                                style="font-size: 24px"
                                v-tooltip
                                :title="
                                    $t('home.battery.low', {
                                        soc: $n(getValue(battery, 'SoC', 100), 'decimalNoDigits'),
                                    })
                                "
                            />
                            <BIconBattery
                                v-else-if="getValue(battery, 'SoC', 100) == 0"
                                style="font-size: 24px; color: red"
                                v-tooltip
                                :title="$t('home.battery.empty')"
                            />
                            <BIconBatteryHalf
                                v-else
                                style="font-size: 24px"
                                v-tooltip
                                :title="
                                    $t('home.battery.level', {
                                        soc: $n(getValue(battery, 'SoC', 100), 'decimalNoDigits'),
                                    })
                                "
                            />
                        </div>
                        <div class="me-2">
                            <span
                                v-if="battery.enabled"
                                class="badge"
                                :class="{
                                    'text-bg-secondary': !battery.poll_enabled,
                                    'text-bg-danger': battery.poll_enabled && !battery.reachable,
                                    'text-bg-warning': battery.poll_enabled && battery.reachable && !battery.producing,
                                    'text-bg-success': battery.poll_enabled && battery.reachable && battery.producing,
                                }"
                            >
                                {{ $n(getValue(battery, 'power'), 'decimalNoDigits') }} W
                            </span>
                            <span v-else class="badge text-bg-light">-</span>
                        </div>
                        <div class="ms-auto me-auto">
                            {{ battery.name }}
                        </div>
                    </div>
                </button>
            </div>
        </div>

        <div
            class="tab-content"
            id="v-pills-battery-tabContent"
            :class="{
                'col-sm-9 col-md-10': batteryData.length > 1,
                'col-sm-12 col-md-12': batteryData.length == 1,
            }"
        >
            <div
                v-for="battery in batteryData"
                :key="battery.uid"
                class="tab-pane fade show"
                :id="'v-pills-battery-' + battery.uid"
                role="tabpanel"
                :aria-labelledby="'v-pills-battery-' + battery.uid + '-tab'"
                tabindex="0"
            >
                <div class="card">
                    <div
                        class="card-header d-flex justify-content-between align-items-center"
                        :class="{
                            'text-bg-tertiary': !battery.poll_enabled,
                            'text-bg-danger': battery.poll_enabled && !battery.reachable,
                            'text-bg-warning': battery.poll_enabled && battery.reachable && !battery.producing,
                            'text-bg-success': battery.poll_enabled && battery.reachable && battery.producing,
                        }"
                    >
                        <div class="p-1 flex-grow-1">
                            <div class="d-flex flex-wrap">
                                <div style="padding-right: 2em">
                                    {{ battery.name }}
                                </div>
                                <div style="padding-right: 2em">{{ $t('home.SerialNumber') }}{{ battery.serial }}</div>
                                <div v-if="battery.limit_absolute >= 0" style="padding-right: 2em">
                                    {{ $t('home.CurrentLimit') }}: {{ $n(battery.limit_absolute, 'decimalNoDigits') }} W
                                </div>
                                <div style="padding-right: 2em">
                                    <DataAgeDisplay
                                        :data-age-ms="battery.data_age_ms"
                                        :threshold-ms="battery.max_age * 1000"
                                    />
                                </div>
                            </div>
                        </div>
                        <!-- Action Buttons -->
                        <div class="btn-toolbar p-2" role="toolbar">
                            <div class="btn-group me-2" role="group">
                                <button
                                    type="button"
                                    class="btn btn-sm btn-info"
                                    @click="onShowBatterySettings(battery)"
                                    v-tooltip
                                    :title="$t('home.showBatterSettings')"
                                >
                                    <BIconInfoCircle style="font-size: 24px" />
                                </button>
                            </div>
                        </div>
                    </div>

                    <div class="card-body">
                        <div class="row flex-row flex-wrap align-items-start g-3">
                            <template v-for="(values, section) in battery.values" v-bind:key="section">
                                <div v-if="section.toString() != 'settings'" v-bind:key="section" class="col order-0">
                                    <div class="card-header text-bg-info">
                                        <template v-if="section.toString().startsWith('_')">
                                            {{ section.toString().substring(1) }}
                                        </template>
                                        <template v-else>
                                            {{ $t('battery.' + section) }}
                                        </template>
                                    </div>
                                    <div class="card-body">
                                        <div class="table-responsive">
                                            <table class="table table-striped table-hover">
                                                <thead>
                                                    <tr>
                                                        <th scope="col">{{ $t('battery.Property') }}</th>
                                                        <th class="value" scope="col">
                                                            {{ $t('battery.Value') }}
                                                        </th>
                                                        <th scope="col">{{ $t('battery.Unit') }}</th>
                                                    </tr>
                                                </thead>
                                                <tbody>
                                                    <tr v-for="(prop, key) in values" v-bind:key="key">
                                                        <th scope="row">{{ $t('battery.' + key) }}</th>
                                                        <td class="value">
                                                            <template v-if="isStringValue(prop) && prop.translate">
                                                                {{ $t('battery.' + prop.value) }}
                                                            </template>
                                                            <template v-else-if="isStringValue(prop)">
                                                                {{ prop.value }}
                                                            </template>
                                                            <template v-else>
                                                                {{
                                                                    $n(prop.v, 'decimal', {
                                                                        minimumFractionDigits: prop.d,
                                                                        maximumFractionDigits: prop.d,
                                                                    })
                                                                }}
                                                            </template>
                                                        </td>
                                                        <td>
                                                            <template v-if="!isStringValue(prop)">
                                                                {{ prop.u }}
                                                            </template>
                                                        </td>
                                                    </tr>
                                                </tbody>
                                            </table>
                                        </div>
                                    </div>
                                </div>
                            </template>
                            <div class="col order-1" v-show="battery.showIssues">
                                <div class="card card-table">
                                    <div :class="{ 'card-header': true, 'border-bottom-0': maxIssueValue === 0 }">
                                        <div class="d-flex flex-row justify-content-between align-items-baseline">
                                            {{ $t('battery.issues') }}
                                            <div v-if="maxIssueValue === 0" class="badge text-bg-success">
                                                {{ $t('battery.noIssues') }}
                                            </div>
                                            <div
                                                v-else-if="maxIssueValue === 1"
                                                class="badge text-bg-warning text-dark"
                                            >
                                                {{ $t('battery.warning') }}
                                            </div>
                                            <div v-else-if="maxIssueValue === 2" class="badge text-bg-danger">
                                                {{ $t('battery.alarm') }}
                                            </div>
                                        </div>
                                    </div>
                                    <div class="card-body" v-if="'issues' in battery">
                                        <table class="table table-striped table-hover">
                                            <thead>
                                                <tr>
                                                    <th scope="col">{{ $t('battery.issueName') }}</th>
                                                    <th scope="col">{{ $t('battery.issueType') }}</th>
                                                </tr>
                                            </thead>
                                            <tbody>
                                                <tr v-for="(prop, key) in battery.issues" v-bind:key="key">
                                                    <th scope="row">{{ $t('battery.' + key) }}</th>
                                                    <td>
                                                        <span
                                                            class="badge"
                                                            :class="{
                                                                'text-bg-warning text-dark': prop === 1,
                                                                'text-bg-danger': prop === 2,
                                                            }"
                                                        >
                                                            <template v-if="prop === 1">{{
                                                                $t('battery.warning')
                                                            }}</template>
                                                            <template v-else>{{ $t('battery.alarm') }}</template>
                                                        </span>
                                                    </td>
                                                </tr>
                                            </tbody>
                                        </table>
                                    </div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
    <ModalDialog
        modalId="batterySettingView"
        :title="$t('home.batterySettings', { name: selectedBattery.name })"
        :loading="batterySettingLoading"
    >
        <!-- <div class="row mb-3 align-items-center">-->
        <CardElement :text="$t('battery.info')" textVariant="text-bg-primary">
            <div class="table-responsive">
                <table class="table table-striped table-hover">
                    <thead>
                        <tr>
                            <th scope="col">{{ $t('battery.Property') }}</th>
                            <th class="value" scope="col">
                                {{ $t('battery.Value') }}
                            </th>
                            <th scope="col"></th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr>
                            <th scope="row">{{ $t('battery.manufacturer') }}</th>
                            <td class="value">{{ selectedBattery.manufacturer }}</td>
                            <td></td>
                        </tr>
                        <tr>
                            <th scope="row">{{ $t('battery.hwversion') }}</th>
                            <td class="value">{{ selectedBattery.hwversion }}</td>
                            <td></td>
                        </tr>
                        <tr>
                            <th scope="row">{{ $t('battery.serial') }}</th>
                            <td class="value">{{ selectedBattery.serial }}</td>
                            <td></td>
                        </tr>
                        <tr>
                            <th scope="row">{{ $t('battery.fwversion') }}</th>
                            <td class="value">{{ selectedBattery.fwversion }}</td>
                            <td></td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </CardElement>
        <CardElement :text="$t('battery.settings')" textVariant="text-bg-primary" addSpace>
            <div class="table-responsive">
                <table class="table table-striped table-hover">
                    <thead>
                        <tr>
                            <th scope="col">{{ $t('battery.Property') }}</th>
                            <th class="value" scope="col">
                                {{ $t('battery.Value') }}
                            </th>
                            <th scope="col">{{ $t('battery.Unit') }}</th>
                        </tr>
                    </thead>
                    <tbody>
                        <template v-for="(values, section) in selectedBattery.values" v-bind:key="section">
                            <template v-if="section.toString() == 'settings'">
                                <tr v-for="(prop, key) in values" v-bind:key="key">
                                    <th scope="row">{{ $t('battery.' + key) }}</th>
                                    <td class="value">
                                        <template v-if="isStringValue(prop) && prop.translate">
                                            {{ $t('battery.' + prop.value) }}
                                        </template>
                                        <template v-else-if="isStringValue(prop)">
                                            {{ prop.value }}
                                        </template>
                                        <template v-else>
                                            {{
                                                $n(prop.v, 'decimal', {
                                                    minimumFractionDigits: prop.d,
                                                    maximumFractionDigits: prop.d,
                                                })
                                            }}
                                        </template>
                                    </td>
                                    <td>
                                        <template v-if="!isStringValue(prop)">
                                            {{ prop.u }}
                                        </template>
                                    </td>
                                </tr>
                            </template>
                        </template>
                    </tbody>
                </table>
            </div>
        </CardElement>
    </ModalDialog>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import type { BatteryInstance, Batteries } from '@/types/BatteryDataStatus';
import { isStringValue } from '@/types/StringValue';
import { handleResponse, authHeader, authUrl } from '@/utils/authentication';
import DataAgeDisplay from '@/components/DataAgeDisplay.vue';
import ModalDialog from '@/components/ModalDialog.vue';
import CardElement from '@/components/CardElement.vue';
import * as bootstrap from 'bootstrap';
import {
    BIconInfoCircle,
    BIconBattery,
    BIconBatteryHalf,
    BIconBatteryFull,
    BIconBatteryCharging,
} from 'bootstrap-icons-vue';
import WebSocketService from '@/utils/websocketService';

export default defineComponent({
    components: {
        CardElement,
        DataAgeDisplay,
        ModalDialog,
        BIconInfoCircle,
        BIconBattery,
        BIconBatteryHalf,
        BIconBatteryFull,
        BIconBatteryCharging,
    },
    data() {
        return {
            socket: {} as WebSocketService,
            heartInterval: 0,
            dataAgeInterval: 0,
            dataLoading: true,
            batteryData: {} as Batteries,
            isFirstFetchAfterConnect: true,

            alertMessageLimit: '',
            alertTypeLimit: 'info',
            showAlertLimit: false,
            checked: false,

            selectedBattery: {} as BatteryInstance,

            batterySettingLoading: false,
            batterySettingView: {} as bootstrap.Modal,
        };
    },
    created() {
        this.getInitialData();
        this.initSocket();
        this.initDataAgeing();
    },
    mounted() {
        this.batterySettingView = new bootstrap.Modal('#batterySettingView');
    },
    unmounted() {
        this.socket?.close();
        clearInterval(this.dataAgeInterval);
    },
    updated() {
        console.log('Updated');
        // Select first tab
        if (this.isFirstFetchAfterConnect) {
            console.log('isFirstFetchAfterConnect');

            this.$nextTick(() => {
                console.log('nextTick');
                const firstTabEl = document.querySelector('#v-pills-battery-tab:first-child button');
                if (firstTabEl != null) {
                    this.isFirstFetchAfterConnect = false;
                    console.log('Show');
                    const firstTab = new bootstrap.Tab(firstTabEl);
                    firstTab.show();
                }
            });
        }
    },
    methods: {
        isStringValue,
        getInitialData() {
            console.log('Get initalData for Battery');
            this.dataLoading = true;

            fetch('/api/batterylivedata/status', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.batteryData = data;
                    this.dataLoading = false;
                });
        },
        handleMessage(event: MessageEvent) {
            console.log(event);
            this.batteryData = JSON.parse(event.data);
            this.dataLoading = false;
        },
        initSocket() {
            console.log('Starting connection to Battery WebSocket Server');

            const { protocol, host } = location;
            const authString = authUrl();
            const webSocketUrl = `${protocol === 'https:' ? 'wss' : 'ws'}://${authString}${host}/batterylivedata`;

            this.socket = new WebSocketService(webSocketUrl, {
                onMessage: this.handleMessage,
                onOpen: () => {
                    console.log('Battery WebSocket connected');
                },
                onClose: () => {
                    console.log('Battery WebSocket closed');
                },
            });

            // Listen to window events , When the window closes , Take the initiative to disconnect websocket Connect
            window.onbeforeunload = () => {
                this.socket?.close();
            };
        },
        initDataAgeing() {
            this.dataAgeInterval = setInterval(() => {
                for (const battery of this.batteryData) {
                    if (battery && battery.uid) {
                        battery.data_age_ms = battery.data_age_ms + 1000;
                    }
                }
            }, 1000);
        },
        onShowBatterySettings(battery: BatteryInstance) {
            this.selectedBattery = battery;
            this.batterySettingView.show();
        },
        getValue(battery: BatteryInstance, name: string, fallback: number = 0, section: string = 'status'): number {
            if (!battery || !name || !section || !battery.values) {
                return fallback;
            }
            const value = battery.values[section]?.[name];
            if (value === undefined || isStringValue(value)) {
                return fallback;
            }

            return value.v === undefined ? fallback : value.v;
        },
    },
    computed: {
        maxIssueValue(battery: BatteryInstance) {
            return 'issues' in battery ? Math.max(...Object.values(battery.issues)) : 0;
        },
    },
});
</script>
