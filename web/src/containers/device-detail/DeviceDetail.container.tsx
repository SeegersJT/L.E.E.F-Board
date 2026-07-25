import { useEffect, useState } from 'react'
import { useParams, useOutletContext } from 'react-router-dom'
import { useAppSelector } from '@/hooks/useAppSelector'
import { useAppDispatch } from '@/hooks/useAppDispatch'
import { requestDeviceHistory } from '@/redux/actions/DeviceHistory.action'
import { requestCommand, clearCommand } from '@/redux/actions/Command.action'
import { deviceCommandKey, isCommandInFlight, isCommandExecuting } from '@/utils/Command'
import type { DashboardOutletContext } from '@/components/dashboard/Dashboard.component'
import StatusPill from '@/components/status-pill/StatusPill.component'
import { listChannels } from '@/utils/Devices'
import DeviceDetail, {
	RANGE_MS,
	type Range,
} from '@/components/device-detail/DeviceDetails.component'

const WATER_NOW = 'WATER_NOW'

function DeviceDetailContainer() {
	const { deviceId } = useParams<{ deviceId: string }>()
	const { setHeaderRight, setTitle } = useOutletContext<DashboardOutletContext>()
	const dispatch = useAppDispatch()

	const devices = useAppSelector(state => state.devices.devices)
	const historyState = useAppSelector(state => state.deviceHistory)

	const commandId = useAppSelector(state =>
		deviceId
			? state.commands.latestKeyByDeviceAndType[deviceCommandKey(deviceId, WATER_NOW)]
			: undefined
	)
	const command = useAppSelector(state =>
		commandId ? state.commands.byId[commandId] : undefined
	)

	const [range, setRange] = useState<Range>('24h')
	const [removeOpen, setRemoveOpen] = useState(false)

	const device = devices?.find(d => d.id === deviceId) ?? null
	const channels = listChannels(device?.status)
	const moisture = channels.find(c => c.type === 'moisture')?.data
	const relay = channels.find(c => c.type === 'relay')?.data

	const sending = isCommandInFlight(command?.phase)
	const watering = isCommandExecuting(command?.phase)

	useEffect(() => {
		setTitle(device?.nickname || 'L.E.E.F. Device')
		setHeaderRight(<StatusPill lastSeen={device?.status?.lastSeen} />)

		return () => {
			setTitle(null)
			setHeaderRight(null)
		}
	}, [device?.nickname, device?.status?.lastSeen, setTitle, setHeaderRight])

	useEffect(() => {
		if (!deviceId) return
		dispatch(requestDeviceHistory({ deviceId, sinceMs: Date.now() - RANGE_MS[range] }))
	}, [deviceId, range, dispatch])

	useEffect(() => {
		return () => {
			if (deviceId) dispatch(clearCommand({ deviceId, type: WATER_NOW }))
		}
	}, [deviceId, dispatch])

	const onWaterNow = () => {
		if (!deviceId) return
		dispatch(requestCommand({ deviceId, type: WATER_NOW }))
	}

	return (
		<DeviceDetail
			loading={devices === null}
			device={device}
			moisture={moisture}
			relay={relay}
			sending={sending}
			watering={watering}
			onWaterNow={onWaterNow}
			range={range}
			onRangeChange={setRange}
			historyLoading={historyState.loading}
			historyError={historyState.error}
			moistureHistory={historyState.deviceId === deviceId ? historyState.moisture : []}
			relayHistory={historyState.deviceId === deviceId ? historyState.relay : []}
			removeOpen={removeOpen}
			onRequestRemove={() => setRemoveOpen(true)}
			onRemoveOpenChange={setRemoveOpen}
		/>
	)
}

export default DeviceDetailContainer
