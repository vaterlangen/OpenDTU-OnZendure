<template>
    <BasePage :title="$t('batteryadmin.batterySettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="alert.show" dismissible :variant="alert.type" :auto-dismiss="0">
            {{ alert.message }}
        </BootstrapAlert>

        <CardElement :text="$t('batteryadmin.addBattery')" textVariant="text-bg-primary">
            <form class="form-inline" v-on:submit.prevent="onSubmit">
                <div class="form-group">
                    <label>{{ $t('batteryadmin.provider') }}</label>
                    <div class="col-sm-8">
                        <select class="form-select" v-model="newBatteryData.provider">
                            <option v-if="batteries.length > 1" :key="7" :value="7">
                                {{ $t('batteryadmin.provider.ZendureMqtt') }}
                            </option>
                            <option
                                v-else
                                v-for="provider in providerTypeList"
                                :key="provider.key"
                                :value="provider.key"
                            >
                                {{ $t('batteryadmin.provider.' + provider.value) }}
                            </option>
                        </select>
                    </div>
                </div>
                <div class="form-group">
                    <label>{{ $t('batteryadmin.name') }}</label>
                    <input
                        v-model="newBatteryData.name"
                        type="text"
                        class="form-control ml-sm-2 mr-sm-4 my-2"
                        maxlength="31"
                        required
                    />
                </div>
                <div class="d-flex my-3">
                    <button type="submit" class="btn btn-primary ms-auto">
                        {{ $t('batteryadmin.add') }}
                    </button>
                </div>
            </form>
            <div class="alert alert-secondary" role="alert" v-html="$t('batteryadmin.addHint')"></div>
        </CardElement>

        <CardElement :text="$t('batteryadmin.batteryList')" textVariant="text-bg-primary" add-space>
            <div class="table-responsive">
                <table class="table">
                    <thead>
                        <tr>
                            <th>#</th>
                            <th scope="col">{{ $t('batteryadmin.status') }}</th>
                            <th>{{ $t('batteryadmin.name') }}</th>
                            <th>{{ $t('batteryadmin.type') }}</th>
                            <th>{{ $t('batteryadmin.action') }}</th>
                        </tr>
                    </thead>
                    <tbody ref="batList">
                        <tr v-for="battery in batteries" v-bind:key="battery.id" :data-id="battery.id">
                            <td><BIconGripHorizontal class="drag-handle" /></td>
                            <td>
                                <span
                                    class="badge"
                                    :title="$t('batteryadmin.enabled')"
                                    :class="{
                                        'text-bg-warning': !battery.enabled,
                                        'text-bg-success': battery.enabled,
                                    }"
                                    ><BIconCheckCircleFill v-if="battery.enabled" /><BIconXCircleFill v-else />
                                </span>
                            </td>
                            <td>{{ battery.name }}</td>
                            <td>
                                {{
                                    $t(
                                        'batteryadmin.provider.' +
                                            providerTypeList.find((p) => p.key === battery.provider)?.value || 'unknown'
                                    )
                                }}
                            </td>
                            <td>
                                <a href="#" class="icon text-danger" :title="$t('batteryadmin.deleteBattery')">
                                    <BIconTrash v-on:click="onOpenModal(modalDelete, battery)" /> </a
                                >&nbsp;
                                <a href="#" class="icon" :title="$t('batteryadmin.editBattery')">
                                    <BIconPencil v-on:click="onOpenModal(modal, battery)" />
                                </a>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
            <div class="d-flex mt-1 mb-3">
                <button class="btn btn-primary ms-auto" @click="onSaveOrder()">
                    {{ $t('batteryadmin.saveOrder') }}
                </button>
            </div>
        </CardElement>
    </BasePage>

    <ModalDialog modalId="batteryEdit" :title="$t('batteryadmin.editBattery')" :closeText="$t('batteryadmin.cancel')">
        <nav>
            <div class="nav nav-tabs" id="nav-tab" role="tablist">
                <button
                    class="nav-link active"
                    id="nav-general-tab"
                    data-bs-toggle="tab"
                    data-bs-target="#nav-general"
                    type="button"
                    role="tab"
                    aria-controls="nav-general"
                    aria-selected="true"
                >
                    {{ $t('batteryadmin.general') }}
                </button>
                <button
                    class="nav-link"
                    id="nav-connection-tab"
                    data-bs-toggle="tab"
                    data-bs-target="#nav-connection"
                    type="button"
                    role="tab"
                    aria-controls="nav-connection"
                >
                    {{ $t('batteryadmin.connection') }}
                </button>
                <button
                    class="nav-link"
                    id="nav-settings-tab"
                    data-bs-toggle="tab"
                    data-bs-target="#nav-settings"
                    type="button"
                    role="tab"
                    aria-controls="nav-settings"
                >
                    {{ $t('batteryadmin.settings') }}
                </button>
                <button
                    class="nav-link"
                    id="nav-extended-tab"
                    data-bs-toggle="tab"
                    data-bs-target="#nav-extended"
                    type="button"
                    role="tab"
                    aria-controls="nav-extended"
                >
                    {{ $t('batteryadmin.extended') }}
                </button>
            </div>
        </nav>
        <div class="tab-content" id="nav-tabContent">
            <div
                class="tab-pane fade show active"
                id="nav-general"
                role="tabpanel"
                aria-labelledby="nav-general-tab"
                tabindex="1"
            >
                <CardElement
                    :text="
                        $t('batteryadmin.generalHeading', {
                            provider: $t(
                                'batteryadmin.provider.' +
                                    providerTypeList.find((p) => p.key === selectedBatteryData.provider)?.value ||
                                    'unknown'
                            ),
                            // uid: '0x' + selectedBatteryData.uid.toString(16).toUpperCase()
                            uid: selectedBatteryData.uid,
                        })
                    "
                    textVariant="text-bg-primary"
                    addSpace
                >
                    <InputElement
                        :label="$t('batteryadmin.enabled')"
                        v-model="selectedBatteryData.enabled"
                        type="checkbox"
                    />
                    <InputElement
                        :label="$t('batteryadmin.name')"
                        v-model="selectedBatteryData.name"
                        type="text"
                        maxlength="31"
                    />
                </CardElement>

                <CardElement
                    :text="$t('batteryadmin.DischargeCurrentLimitConfiguration')"
                    textVariant="text-bg-primary"
                    addSpace
                >
                    <InputElement
                        :label="$t('batteryadmin.LimitDischargeCurrent')"
                        v-model="selectedBatteryData.enable_discharge_current_limit"
                        type="checkbox"
                        wide
                    />

                    <template v-if="selectedBatteryData.enable_discharge_current_limit">
                        <InputElement
                            :label="$t('batteryadmin.DischargeCurrentLimit')"
                            v-model="selectedBatteryData.discharge_current_limit"
                            type="number"
                            min="0"
                            step="0.1"
                            postfix="A"
                            wide
                        />

                        <InputElement
                            :label="$t('batteryadmin.DischargeCurrentLimitBelowSoc')"
                            v-if="selectedBatteryData.enabled"
                            v-model="selectedBatteryData.discharge_current_limit_below_soc"
                            type="number"
                            min="0"
                            max="100"
                            step="0.1"
                            postfix="%"
                            :tooltip="$t('batteryadmin.DischargeCurrentLimitBelowSocInfo')"
                            wide
                        />

                        <InputElement
                            :label="$t('batteryadmin.DischargeCurrentLimitBelowVoltage')"
                            v-if="selectedBatteryData.enabled"
                            v-model="selectedBatteryData.discharge_current_limit_below_voltage"
                            type="number"
                            min="0"
                            max="60"
                            step="0.01"
                            postfix="V"
                            :tooltip="$t('batteryadmin.DischargeCurrentLimitBelowVoltageInfo')"
                            wide
                        />

                        <template
                            v-if="
                                selectedBatteryData.enabled &&
                                (selectedBatteryData.provider == 0 ||
                                    selectedBatteryData.provider == 2 ||
                                    selectedBatteryData.provider == 4 ||
                                    selectedBatteryData.provider == 5)
                            "
                        >
                            <InputElement
                                :label="$t('batteryadmin.UseBatteryReportedDischargeCurrentLimit')"
                                v-model="selectedBatteryData.use_battery_reported_discharge_current_limit"
                                type="checkbox"
                                wide
                            />

                            <template v-if="selectedBatteryData.use_battery_reported_discharge_current_limit">
                                <div
                                    class="alert alert-secondary"
                                    role="alert"
                                    v-if="selectedBatteryData.enabled"
                                    v-html="$t('batteryadmin.BatteryReportedDischargeCurrentLimitInfo')"
                                ></div>

                                <template v-if="selectedBatteryData.provider == 2">
                                    <InputElement
                                        :label="$t('batteryadmin.MqttDischargeCurrentLimitTopic')"
                                        v-model="selectedBatteryData.mqtt.discharge_current_limit_topic"
                                        wide
                                        type="text"
                                        maxlength="256"
                                    />

                                    <InputElement
                                        :label="$t('batteryadmin.MqttJsonPath')"
                                        v-model="selectedBatteryData.mqtt.discharge_current_limit_json_path"
                                        wide
                                        type="text"
                                        maxlength="256"
                                        :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                                    />

                                    <div class="row mb-3">
                                        <label for="discharge_current_limit_unit" class="col-sm-4 col-form-label">
                                            {{ $t('batteryadmin.MqttAmperageUnit') }}
                                        </label>

                                        <div class="col-sm-8">
                                            <select
                                                id="discharge_current_limit_unit"
                                                class="form-select"
                                                v-model="selectedBatteryData.mqtt.discharge_current_limit_unit"
                                            >
                                                <option v-for="u in amperageUnitTypeList" :key="u.key" :value="u.key">
                                                    {{ u.value }}
                                                </option>
                                            </select>
                                        </div>
                                    </div>
                                </template>
                            </template>
                        </template>
                    </template>
                </CardElement>
            </div>

            <div
                class="tab-pane fade show"
                id="nav-connection"
                role="tabpanel"
                aria-labelledby="nav-connection-tab"
                tabindex="2"
            >
                <template v-if="selectedBatteryData.provider == 1 || selectedBatteryData.provider == 6">
                    <div class="row mb-3">
                        <label class="col-sm-4 col-form-label">
                            {{ $t('batteryadmin.SerialInterfaceType') }}
                        </label>
                        <div class="col-sm-8">
                            <select class="form-select" v-model="selectedBatteryData.serial.interface">
                                <option
                                    v-for="serialInterface in serialBmsInterfaceTypeList"
                                    :key="serialInterface.key"
                                    :value="serialInterface.key"
                                >
                                    {{ $t(`batteryadmin.SerialInterfaceType` + serialInterface.value) }}
                                </option>
                            </select>
                        </div>
                    </div>

                    <InputElement
                        :label="$t('batteryadmin.PollingInterval')"
                        v-model="selectedBatteryData.serial.polling_interval"
                        type="number"
                        min="2"
                        max="90"
                        step="1"
                        :postfix="$t('batteryadmin.Seconds')"
                        wide
                    />
                </template>
                <template v-if="selectedBatteryData.enabled && selectedBatteryData.provider == 2">
                    <CardElement :text="$t('batteryadmin.MqttSocConfiguration')" textVariant="text-bg-primary" addSpace>
                        <InputElement
                            :label="$t('batteryadmin.MqttSocTopic')"
                            v-model="selectedBatteryData.mqtt.soc_topic"
                            type="text"
                            maxlength="256"
                            wide
                        />

                        <InputElement
                            :label="$t('batteryadmin.MqttJsonPath')"
                            v-model="selectedBatteryData.mqtt.soc_json_path"
                            type="text"
                            maxlength="256"
                            :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                            wide
                        />
                    </CardElement>

                    <CardElement
                        :text="$t('batteryadmin.MqttVoltageConfiguration')"
                        textVariant="text-bg-primary"
                        addSpace
                    >
                        <InputElement
                            :label="$t('batteryadmin.MqttVoltageTopic')"
                            v-model="selectedBatteryData.mqtt.voltage_topic"
                            type="text"
                            maxlength="256"
                            wide
                        />

                        <InputElement
                            :label="$t('batteryadmin.MqttJsonPath')"
                            v-model="selectedBatteryData.mqtt.voltage_json_path"
                            type="text"
                            maxlength="256"
                            :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                            wide
                        />

                        <div class="row mb-3">
                            <label for="mqtt_voltage_unit" class="col-sm-4 col-form-label">
                                {{ $t('batteryadmin.MqttVoltageUnit') }}
                            </label>
                            <div class="col-sm-8">
                                <select
                                    id="mqtt_voltage_unit"
                                    class="form-select"
                                    v-model="selectedBatteryData.mqtt.voltage_unit"
                                >
                                    <option v-for="u in voltageUnitTypeList" :key="u.key" :value="u.key">
                                        {{ u.value }}
                                    </option>
                                </select>
                            </div>
                        </div>
                    </CardElement>

                    <CardElement
                        :text="$t('batteryadmin.MqttCurrentConfiguration')"
                        textVariant="text-bg-primary"
                        addSpace
                    >
                        <InputElement
                            :label="$t('batteryadmin.MqttCurrentTopic')"
                            v-model="selectedBatteryData.mqtt.current_topic"
                            type="text"
                            maxlength="256"
                            wide
                        />

                        <InputElement
                            :label="$t('batteryadmin.MqttJsonPath')"
                            v-model="selectedBatteryData.mqtt.current_json_path"
                            type="text"
                            maxlength="256"
                            :tooltip="$t('batteryadmin.MqttJsonPathDescription')"
                            wide
                        />

                        <div class="row mb-3">
                            <label for="mqtt_current_unit" class="col-sm-4 col-form-label">
                                {{ $t('batteryadmin.MqttAmperageUnit') }}
                            </label>
                            <div class="col-sm-8">
                                <select
                                    id="mqtt_current_unit"
                                    class="form-select"
                                    v-model="selectedBatteryData.mqtt.current_unit"
                                >
                                    <option v-for="u in amperageUnitTypeList" :key="u.key" :value="u.key">
                                        {{ u.value }}
                                    </option>
                                </select>
                            </div>
                        </div>
                    </CardElement>
                </template>
                <template v-if="selectedBatteryData.provider == 7">
                    <div class="row mb-3">
                        <label for="zendure_connection_type" class="col-sm-2 col-form-label">
                            {{ $t('batteryadmin.zendure.connectionType') }}
                        </label>
                        <div class="col-sm-10">
                            <select
                                id="zendure_connection_type"
                                class="form-select"
                                v-model="selectedBatteryData.zendure.connection_type"
                            >
                                <option v-for="u in zendureConnectionTypeList" :key="u.key" :value="u.key">
                                    {{ $t('batteryadmin.zendure.connectionTypes.' + u.value) }}
                                </option>
                            </select>
                        </div>
                    </div>

                    <template v-if="selectedBatteryData.zendure.connection_type != 1">
                        <div class="row mb-3">
                            <label for="zendure_device_type" class="col-sm-2 col-form-label">
                                {{ $t('batteryadmin.zendure.deviceType') }}
                            </label>
                            <div class="col-sm-10">
                                <select
                                    id="zendure_device_type"
                                    class="form-select"
                                    v-model="selectedBatteryData.zendure.device_type"
                                >
                                    <option v-for="u in zendureDeviceTypeList" :key="u.key" :value="u.key">
                                        {{ $t('batteryadmin.zendure.deviceTypes.' + u.value) }}
                                    </option>
                                </select>
                            </div>
                        </div>
                    </template>

                    <template v-if="selectedBatteryData.zendure.connection_type != 2">
                        <InputElement
                            :label="$t('batteryadmin.zendureDeviceId')"
                            v-model="selectedBatteryData.zendure.device_id"
                            type="text"
                            maxlength="31"
                        />
                        <div class="row">
                            <div class="col-sm-2"></div>
                            <div class="col-sm-10">
                                <div
                                    v-if="selectedBatteryData.zendure.device_id.length != 8"
                                    class="alert alert-warning"
                                    role="alert"
                                    v-html="$t('batteryadmin.zendure.deviceIdDescription')"
                                ></div>
                            </div>
                        </div>
                    </template>

                    <template v-if="selectedBatteryData.zendure.connection_type == 0">
                        <div class="row">
                            <div class="col-sm-2"></div>
                            <div class="col-sm-10">
                                <div
                                    v-if="selectedBatteryData.zendure.device_id.length == 8"
                                    class="alert alert-info"
                                    role="alert"
                                    v-html="
                                        $t('batteryadmin.zendure.deviceIdInfo', {
                                            user: selectedBatteryData.zendure.device_id,
                                            pass: calcZendurePassword(selectedBatteryData.zendure.device_id),
                                        })
                                    "
                                ></div>
                            </div>
                        </div>
                    </template>

                    <template v-if="selectedBatteryData.zendure.connection_type == 1">
                        <InputElement
                            :label="$t('batteryadmin.zendure.server')"
                            v-model="selectedBatteryData.zendure.server"
                            type="text"
                            minlength="4"
                            maxlength="256"
                            :key="$route.fullPath"
                        />
                        <InputElement
                            :label="$t('batteryadmin.zendure.port')"
                            v-model="selectedBatteryData.zendure.port"
                            type="number"
                            min="1"
                            max="65535"
                            step="1"
                            :key="$route.fullPath"
                        />
                        <InputElement
                            :label="$t('batteryadmin.zendure.clientId')"
                            v-model="selectedBatteryData.zendure.client_id"
                            type="text"
                            minlength="1"
                            maxlength="23"
                            :key="$route.fullPath"
                        />
                        <InputElement
                            :label="$t('batteryadmin.zendure.appKey')"
                            v-model="selectedBatteryData.zendure.app_key"
                            type="text"
                            minlength="8"
                            maxlength="8"
                            :key="$route.fullPath"
                        />
                        <InputElement
                            :label="$t('batteryadmin.zendure.secret')"
                            v-model="selectedBatteryData.zendure.secret"
                            type="password"
                            minlength="32"
                            maxlength="32"
                            :key="$route.fullPath"
                        />
                        <div class="row">
                            <div class="col-sm-2"></div>
                            <div class="col-sm-10">
                                <div
                                    v-if="
                                        selectedBatteryData.zendure.secret.length != 36 &&
                                        selectedBatteryData.zendure.app_key.length != 8
                                    "
                                    class="alert alert-warning"
                                    role="alert"
                                    v-html="$t('batteryadmin.zendure.apiDescription')"
                                ></div>
                            </div>
                        </div>
                    </template>

                    <template
                        v-if="
                            selectedBatteryData.zendure.connection_type == 0 ||
                            selectedBatteryData.zendure.connection_type == 2
                        "
                    >
                        <InputElement
                            :label="$t('batteryadmin.PollingInterval')"
                            v-model="selectedBatteryData.zendure.polling_interval"
                            type="number"
                            min="10"
                            max="120"
                            step="1"
                            :tooltip="$t('batteryadmin.zendure.pollingIntervalDescription')"
                            :postfix="$t('batteryadmin.Seconds')"
                        />
                        <div class="row mb-3">
                            <label for="zendure_control_mode" class="col-sm-2 col-form-label">
                                {{ $t('batteryadmin.zendure.controlMode') }}
                            </label>
                            <div class="col-sm-10">
                                <select
                                    id="zendure_control_mode"
                                    class="form-select"
                                    v-model="selectedBatteryData.zendure.control_mode"
                                    @change="
                                        selectedBatteryData.zendure.output_control = 0;
                                        selectedBatteryData.zendure.charge_through_enable = false;
                                    "
                                >
                                    <option v-for="u in zendureControlModeList" :key="u.key" :value="u.key">
                                        {{ $t('batteryadmin.zendure.controlModes.' + u.value) }}
                                    </option>
                                </select>
                            </div>
                        </div>
                    </template>
                </template>
            </div>

            <div
                class="tab-pane fade show"
                id="nav-settings"
                role="tabpanel"
                aria-labelledby="nav-settings-tab"
                tabindex="3"
            >
                <template v-if="selectedBatteryData.provider == 7">
                    <template
                        v-if="
                            selectedBatteryData.zendure.connection_type == 0 ||
                            selectedBatteryData.zendure.connection_type == 2
                        "
                    >
                        <template
                            v-if="
                                selectedBatteryData.zendure.control_mode == 0 ||
                                selectedBatteryData.zendure.control_mode == 1
                            "
                        >
                            <InputElement
                                :label="$t('batteryadmin.ZendureMaxOutput')"
                                v-model="selectedBatteryData.zendure.max_output"
                                type="number"
                                min="100"
                                max="1200"
                                step="100"
                                :postfix="$t('batteryadmin.Watt')"
                            />
                            <InputElement
                                :label="$t('batteryadmin.ZendureMinSoc')"
                                v-model="selectedBatteryData.zendure.soc_min"
                                type="number"
                                min="0"
                                max="60"
                                step="1"
                                :postfix="$t('batteryadmin.Percent')"
                            />
                            <div class="row">
                                <div class="col-sm-2"></div>
                                <div class="col-sm-10">
                                    <div
                                        v-if="
                                            selectedBatteryData.zendure.soc_min >= 0 &&
                                            selectedBatteryData.zendure.soc_min < 10
                                        "
                                        class="alert alert-warning"
                                        role="alert"
                                        v-html="
                                            $t('batteryadmin.zendure.socMinWarning', {
                                                soc: selectedBatteryData.zendure.soc_min,
                                                min: 10,
                                            })
                                        "
                                    ></div>
                                </div>
                            </div>
                            <InputElement
                                :label="$t('batteryadmin.ZendureMaxSoc')"
                                v-model="selectedBatteryData.zendure.soc_max"
                                type="number"
                                min="40"
                                max="100"
                                step="1"
                                :postfix="$t('batteryadmin.Percent')"
                            />
                            <div class="row mb-3">
                                <label for="zendure_bypass_mode" class="col-sm-2 col-form-label">
                                    {{ $t('batteryadmin.ZendureBypassMode') }}
                                </label>
                                <div class="col-sm-10">
                                    <select
                                        id="zendure_bypass_mode"
                                        class="form-select"
                                        v-model="selectedBatteryData.zendure.bypass_mode"
                                    >
                                        <option v-for="u in zendureBypassModeList" :key="u.key" :value="u.key">
                                            {{ $t(`batteryadmin.ZendureBypassMode` + u.value) }}
                                        </option>
                                    </select>
                                </div>
                            </div>
                            <InputElement
                                :label="$t('batteryadmin.ZendureAutoShutdown')"
                                v-model="selectedBatteryData.zendure.auto_shutdown"
                                type="checkbox"
                                :tooltip="$t('batteryadmin.ZendureAutoShutdownDescription')"
                            />
                            <InputElement
                                :label="$t('batteryadmin.zendure.buzzerEnable')"
                                v-model="selectedBatteryData.zendure.buzzer_enable"
                                type="checkbox"
                            />
                        </template>
                    </template>
                </template>
            </div>

            <div
                class="tab-pane fade show pt-3"
                id="nav-extended"
                role="tabpanel"
                aria-labelledby="nav-extended-tab"
                tabindex="4"
            >
                <template v-if="selectedBatteryData.provider == 7">
                    <template
                        v-if="
                            selectedBatteryData.zendure.connection_type == 0 ||
                            selectedBatteryData.zendure.connection_type == 2
                        "
                    >
                        <template
                            v-if="
                                selectedBatteryData.zendure.control_mode == 0 &&
                                selectedBatteryData.zendure.output_control != 0
                            "
                        >
                            <CardElement
                                :text="$t('batteryadmin.zendure.batteryProtection')"
                                textVariant="text-bg-primary"
                                addSpace
                            >
                                <InputElement
                                    :label="$t('batteryadmin.zendure.batteryProtectionEnabled')"
                                    v-model="selectedBatteryData.zendure.battery_protection_enable"
                                    type="checkbox"
                                />
                                <template v-if="selectedBatteryData.zendure.battery_protection_enable">
                                    <InputElement
                                        :label="$t('batteryadmin.zendure.socMinHysteresis')"
                                        v-model="selectedBatteryData.zendure.min_soc_hysteresis"
                                        type="number"
                                        min="0"
                                        max="25"
                                        step="0.1"
                                        :postfix="$t('batteryadmin.Percent')"
                                    />
                                </template>
                            </CardElement>
                            <CardElement
                                :text="$t('batteryadmin.zendure.chargeThrough')"
                                textVariant="text-bg-primary"
                                addSpace
                            >
                                <InputElement
                                    :label="$t('batteryadmin.zendure.chargeThroughEnabled')"
                                    v-model="selectedBatteryData.zendure.charge_through_enable"
                                    type="checkbox"
                                />
                                <template v-if="selectedBatteryData.zendure.charge_through_enable">
                                    <InputElement
                                        :label="$t('batteryadmin.zendure.chargeThroughInterval')"
                                        v-model="selectedBatteryData.zendure.charge_through_interval"
                                        type="number"
                                        min="0"
                                        max="8766"
                                        step="1"
                                        :postfix="$t('batteryadmin.Hours')"
                                    />
                                    <InputElement
                                        :label="$t('batteryadmin.zendure.chargeThroughReset')"
                                        v-model="selectedBatteryData.zendure.charge_through_reset"
                                        type="number"
                                        min="25"
                                        max="100"
                                        step="1"
                                        :postfix="$t('batteryadmin.Percent')"
                                    />
                                </template>
                            </CardElement>
                        </template>
                        <CardElement
                            :text="$t('batteryadmin.ZendureOutputControl')"
                            textVariant="text-bg-primary"
                            addSpace
                        >
                            <div class="row mb-3">
                                <label for="zendure_output_mode" class="col-sm-2 col-form-label">
                                    {{ $t('batteryadmin.Mode') }}
                                </label>
                                <div class="col-sm-10">
                                    <select
                                        id="zendure_output_mode"
                                        class="form-select"
                                        v-model="selectedBatteryData.zendure.output_control"
                                        @change="
                                            selectedBatteryData.zendure.charge_through_enable = false;
                                            selectedBatteryData.zendure.battery_protection_enable = false;
                                        "
                                    >
                                        <option :key="0" :value="0">
                                            {{
                                                $t(
                                                    'batteryadmin.ZendureOutputMode' +
                                                        zendureOutputControlList[0]?.value
                                                )
                                            }}
                                        </option>
                                        <option
                                            :key="1"
                                            :value="1"
                                            v-if="
                                                selectedBatteryData.zendure.control_mode == 0 ||
                                                selectedBatteryData.zendure.control_mode == 1
                                            "
                                        >
                                            {{
                                                $t(
                                                    'batteryadmin.ZendureOutputMode' +
                                                        zendureOutputControlList[1]?.value
                                                )
                                            }}
                                        </option>
                                        <option
                                            :key="2"
                                            :value="2"
                                            v-if="selectedBatteryData.zendure.control_mode == 0"
                                        >
                                            {{
                                                $t(
                                                    'batteryadmin.ZendureOutputMode' +
                                                        zendureOutputControlList[2]?.value
                                                )
                                            }}
                                        </option>
                                    </select>
                                </div>
                            </div>

                            <template v-if="selectedBatteryData.zendure.output_control == 1">
                                <InputElement
                                    :label="$t('batteryadmin.ZendureOutputLimit')"
                                    v-model="selectedBatteryData.zendure.output_limit"
                                    type="number"
                                    min="0"
                                    max="1200"
                                    step="1"
                                    :postfix="$t('batteryadmin.Watt')"
                                />
                            </template>

                            <template v-if="selectedBatteryData.zendure.output_control == 2">
                                <InputElement
                                    :label="$t('batteryadmin.ZendureSunriseOffset')"
                                    v-model="selectedBatteryData.zendure.sunrise_offset"
                                    type="number"
                                    min="-360"
                                    max="360"
                                    step="1"
                                    :postfix="$t('batteryadmin.Minutes')"
                                />
                                <InputElement
                                    :label="$t('batteryadmin.ZendureOutputLimitDay')"
                                    v-model="selectedBatteryData.zendure.output_limit_day"
                                    type="number"
                                    min="0"
                                    max="1200"
                                    step="1"
                                    :postfix="$t('batteryadmin.Watt')"
                                />
                                <InputElement
                                    :label="$t('batteryadmin.ZendureSunsetOffset')"
                                    v-model="selectedBatteryData.zendure.sunset_offset"
                                    type="number"
                                    min="-360"
                                    max="360"
                                    step="1"
                                    :postfix="$t('batteryadmin.Minutes')"
                                />
                                <InputElement
                                    :label="$t('batteryadmin.ZendureOutputLimitNight')"
                                    v-model="selectedBatteryData.zendure.output_limit_night"
                                    type="number"
                                    min="0"
                                    max="1200"
                                    step="1"
                                    :postfix="$t('batteryadmin.Watt')"
                                />
                            </template>
                        </CardElement>
                    </template>
                </template>
            </div>
        </div>
        <template #footer>
            <button type="button" class="btn btn-primary" @click="onEditSubmit">
                {{ $t('batteryadmin.save') }}
            </button>
        </template>
    </ModalDialog>

    <ModalDialog
        modalId="batteryDelete"
        small
        :title="$t('batteryadmin.deleteInverter')"
        :closeText="$t('batteryadmin.cancel')"
    >
        {{
            $t('batteryadmin.deleteMsg', {
                name: selectedBatteryData.name,
                serial: '',
            })
        }}
        <template #footer>
            <button type="button" class="btn btn-danger" @click="onDelete">
                {{ $t('batteryadmin.delete') }}
            </button>
        </template>
    </ModalDialog>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from '@/components/BootstrapAlert.vue';
import CardElement from '@/components/CardElement.vue';
import InputElement from '@/components/InputElement.vue';
import ModalDialog from '@/components/ModalDialog.vue';
import type { AlertResponse } from '@/types/AlertResponse';
import type { BatteryConfig } from '@/types/BatteryConfig';
import { authHeader, handleResponse } from '@/utils/authentication';
import md5 from 'spark-md5';
import * as bootstrap from 'bootstrap';
import {
    BIconGripHorizontal,
    BIconPencil,
    BIconTrash,
    BIconXCircleFill,
    BIconCheckCircleFill,
} from 'bootstrap-icons-vue';
import Sortable from 'sortablejs';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        InputElement,
        ModalDialog,
        BIconPencil,
        BIconTrash,
        BIconGripHorizontal,
        BIconXCircleFill,
        BIconCheckCircleFill,
    },
    data() {
        return {
            modal: {} as bootstrap.Modal,
            modalDelete: {} as bootstrap.Modal,
            newBatteryData: { uid: 0 } as BatteryConfig,
            selectedBatteryData: { uid: 0 } as BatteryConfig,
            batteries: [] as BatteryConfig[],
            dataLoading: true,
            alert: {} as AlertResponse,
            sortable: {} as Sortable,
            providerTypeList: [
                { key: 0, value: 'PylontechCan' },
                { key: 1, value: 'JkBmsSerial' },
                { key: 2, value: 'Mqtt' },
                { key: 3, value: 'Victron' },
                { key: 4, value: 'PytesCan' },
                { key: 5, value: 'SBSCan' },
                { key: 6, value: 'JbdBmsSerial' },
                { key: 7, value: 'ZendureMqtt' },
            ],
            serialBmsInterfaceTypeList: [
                { key: 0, value: 'Uart' },
                { key: 1, value: 'Transceiver' },
            ],
            voltageUnitTypeList: [
                { key: 3, value: 'mV' },
                { key: 2, value: 'cV' },
                { key: 1, value: 'dV' },
                { key: 0, value: 'V' },
            ],
            amperageUnitTypeList: [
                { key: 1, value: 'mA' },
                { key: 0, value: 'A' },
            ],
            zendureDeviceTypeList: [
                { key: 0, value: 'Hub1200' },
                { key: 1, value: 'Hub2000' },
                { key: 2, value: 'AIO2400' },
                { key: 3, value: 'Ace1500' },
                { key: 4, value: 'Hyper2000A' },
                { key: 5, value: 'Hyper2000B' },
            ],
            zendureBypassModeList: [
                { key: 0, value: 'Automatic' },
                { key: 1, value: 'AlwaysOff' },
                { key: 2, value: 'AlwaysOn' },
            ],
            zendureOutputControlList: [
                { key: 0, value: 'External' },
                { key: 1, value: 'Fixed' },
                { key: 2, value: 'Schedule' },
            ],
            zendureControlModeList: [
                { key: 0, value: 'Full' },
                { key: 1, value: 'Once' },
                { key: 2, value: 'ReadOnly' },
            ],
            zendureConnectionTypeList: [
                { key: 0, value: 'local' },
                { key: 1, value: 'cloud' },
                //{ key: 2, value: 'bluetooth' },
            ],
        };
    },
    mounted() {
        this.modal = new bootstrap.Modal('#batteryEdit');
        this.modalDelete = new bootstrap.Modal('#batteryDelete');
    },
    created() {
        this.getBatteries();
    },
    methods: {
        calcZendurePassword(id: string) {
            return md5.hash(id).toUpperCase().substring(8, 24);
        },
        getBatteries() {
            this.dataLoading = true;
            fetch('/api/battery/list', { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.batteries = data.battery.slice().sort((a: BatteryConfig, b: BatteryConfig) => {
                        return a.order - b.order;
                    });
                    this.dataLoading = false;

                    this.$nextTick(() => {
                        const table = this.$refs.batList as HTMLElement;

                        this.sortable = Sortable.create(table, {
                            sort: true,
                            handle: '.drag-handle',
                            animation: 150,
                            draggable: 'tr',
                        });
                    });
                });
        },
        callBatteryApiEndpoint(endpoint: string, jsonData: string) {
            const formData = new FormData();
            formData.append('data', jsonData);

            fetch('/api/battery/' + endpoint, {
                method: 'POST',
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    this.getBatteries();
                    this.alert = data;
                    this.alert.message = this.$t('apiresponse.' + data.code, data.param);
                    this.alert.show = true;
                });
        },
        onSubmit() {
            this.newBatteryData.uid = Math.floor(Math.random() * 0x7fffffff) | 0x40000000;
            this.newBatteryData.enabled = false;
            this.callBatteryApiEndpoint('add', JSON.stringify(this.newBatteryData));
            this.newBatteryData = { uid: 0 } as BatteryConfig;
        },
        onDelete() {
            this.callBatteryApiEndpoint('del', JSON.stringify({ id: this.selectedBatteryData.id }));
            this.onCloseModal(this.modalDelete);
        },
        onEditSubmit() {
            this.callBatteryApiEndpoint('edit', JSON.stringify(this.selectedBatteryData));
            this.onCloseModal(this.modal);
        },
        onOpenModal(modal: bootstrap.Modal, battery: BatteryConfig) {
            // deep copy battery object for editing/deleting
            this.selectedBatteryData = JSON.parse(JSON.stringify(battery)) as BatteryConfig;
            modal.show();
        },
        onCloseModal(modal: bootstrap.Modal) {
            modal.hide();
        },
        onSaveOrder() {
            this.callBatteryApiEndpoint('order', JSON.stringify({ order: this.sortable.toArray() }));
        },
    },
});
</script>

<style scoped>
.drag-handle {
    cursor: grab;
}
</style>
