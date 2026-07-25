import {
	ref,
	onValue,
	get,
	update,
	query,
	orderByKey,
	startAt,
	type DataSnapshot,
	push,
	set,
} from 'firebase/database'
import { rtdb } from './config'
import type { DeviceStatus } from '@/redux/types/Device.type'
import type { MoistureHistoryPoint, RelayHistoryPoint } from '@/redux/types/DeviceHistory.type'
import { serverTimestamp } from 'firebase/firestore'

export interface UserDeviceEntry {
	id: string
	nickname: string | null
}

const normalizeUserDeviceEntries = (val: Record<string, unknown> | null): UserDeviceEntry[] => {
	if (!val) return []
	return Object.entries(val).map(([id, raw]) => {
		if (raw && typeof raw === 'object' && 'nickname' in raw) {
			const nickname = (raw as { nickname?: unknown }).nickname
			return { id, nickname: typeof nickname === 'string' ? nickname : null }
		}
		return { id, nickname: null }
	})
}

export interface PairingCodeEntry {
	deviceId: string
}

export interface DeviceHistoryResult {
	moisture: MoistureHistoryPoint[]
	relay: RelayHistoryPoint[]
}

export const rtdbService = {
	subscribeToUserDevices: (uid: string, callback: (entries: UserDeviceEntry[]) => void) => {
		const devicesRef = ref(rtdb, `users/${uid}/devices`)
		return onValue(devicesRef, (snapshot: DataSnapshot) => {
			callback(normalizeUserDeviceEntries(snapshot.val()))
		})
	},

	subscribeToDeviceStatus: (
		deviceId: string,
		callback: (status: DeviceStatus | null) => void
	) => {
		const statusRef = ref(rtdb, `devices/${deviceId}/status`)
		return onValue(statusRef, (snapshot: DataSnapshot) => {
			callback(snapshot.val() as DeviceStatus | null)
		})
	},

	lookupPairingCode: async (code: string): Promise<PairingCodeEntry | null> => {
		try {
			const snapshot = await get(ref(rtdb, `pairingCodes/${code}`))
			if (!snapshot.exists()) return null
			const val = snapshot.val() as { deviceId?: string }
			return val.deviceId ? { deviceId: val.deviceId } : null
		} catch (err) {
			const errorCode = (err as { code?: string }).code ?? ''
			if (errorCode.toUpperCase().includes('PERMISSION_DENIED')) {
				return null
			}
			throw err
		}
	},

	claimDevice: async ({
		code,
		deviceId,
		nickname,
		uid,
	}: {
		code: string
		deviceId: string
		nickname: string
		uid: string
	}): Promise<void> => {
		await update(ref(rtdb), {
			[`devices/${deviceId}/owner`]: { uid, claimedWithCode: code },
			[`users/${uid}/devices/${deviceId}`]: { nickname },
		})
	},

	fetchDeviceHistory: async (deviceId: string, sinceMs: number): Promise<DeviceHistoryResult> => {
		const sinceIso = new Date(sinceMs).toISOString().replace(/\.\d{3}Z$/, 'Z')

		const moistureQuery = query(
			ref(rtdb, `devices/${deviceId}/history/MOISTURE_SENSOR_PIN_MM01`),
			orderByKey(),
			startAt(sinceIso)
		)
		const relayQuery = query(
			ref(rtdb, `devices/${deviceId}/history/RELAY_PIN_R01`),
			orderByKey(),
			startAt(sinceIso)
		)

		const [moistureSnap, relaySnap] = await Promise.all([get(moistureQuery), get(relayQuery)])

		const moisture: MoistureHistoryPoint[] = []
		moistureSnap.forEach(child => {
			const val = child.val() as { reading?: number }
			if (typeof val.reading === 'number') {
				moisture.push({ timestamp: child.key as string, reading: val.reading })
			}
		})

		const relay: RelayHistoryPoint[] = []
		relaySnap.forEach(child => {
			const val = child.val() as { state?: 'ON' | 'OFF' }
			if (val.state === 'ON' || val.state === 'OFF') {
				relay.push({ timestamp: child.key as string, state: val.state })
			}
		})

		return { moisture, relay }
	},

	removeDevice: async ({ deviceId, uid }: { deviceId: string; uid: string }): Promise<void> => {
		await update(ref(rtdb), {
			[`devices/${deviceId}/owner`]: null,
			[`users/${uid}/devices/${deviceId}`]: null,
		})
	},

	sendCommand: async ({
		deviceId,
		uid,
		type,
		params,
	}: {
		deviceId: string
		uid: string
		type: string
		params?: Record<string, unknown>
	}): Promise<string> => {
		const commandsRef = ref(rtdb, `devices/${deviceId}/commands`)
		const newCommandRef = push(commandsRef)
		const commandId = newCommandRef.key

		if (!commandId) {
			throw new Error('Failed to generate a command id')
		}

		await set(newCommandRef, {
			type,
			requestedBy: { kind: 'user', uid },
			requestedAt: serverTimestamp(),
			status: 'pending',
			statusUpdatedAt: serverTimestamp(),
			...(params ? { params } : {}),
		})

		return commandId
	},

	subscribeToCommandStatus: (
		deviceId: string,
		commandId: string,
		callback: (status: string | null) => void
	) => {
		const statusRef = ref(rtdb, `devices/${deviceId}/commands/${commandId}/status`)
		return onValue(statusRef, (snapshot: DataSnapshot) => {
			callback(snapshot.exists() ? (snapshot.val() as string) : null)
		})
	},
}
