import { eventChannel, type EventChannel } from 'redux-saga'
import { call, put, take, select, takeEvery } from 'redux-saga/effects'
import { rtdbService } from '@/firebase'
import {
	COMMAND_ACTIONS,
	commandStarted,
	setCommandPhase,
	commandError,
} from '../actions/Command.action'
import { addSystemNotification } from '../actions/Notification.action'
import type { RootState } from '../types/_Root.type'

function createCommandStatusChannel(deviceId: string, commandId: string) {
	return eventChannel<string | null>(emit =>
		rtdbService.subscribeToCommandStatus(deviceId, commandId, emit)
	)
}

function* watchCommandStatus(deviceId: string, commandId: string) {
	const channel: EventChannel<string | null> = yield call(
		createCommandStatusChannel,
		deviceId,
		commandId
	)

	try {
		while (true) {
			const status: string | null = yield take(channel)
			if (!status) continue

			yield put(setCommandPhase({ commandId, phase: status }))

			const isTerminal =
				status === 'completed' ||
				status === 'failed' ||
				status === 'rejected' ||
				status === 'expired'

			if (status === 'completed') {
				yield put(
					addSystemNotification({
						type: 'success',
						title: 'Command completed',
						message: 'The device finished successfully.',
					})
				)
			} else if (isTerminal) {
				yield put(
					addSystemNotification({
						type: 'error',
						title: 'Command did not complete',
						message:
							status === 'expired'
								? 'The device was busy too long and this request timed out.'
								: 'The device rejected or failed this command.',
					})
				)
			}

			if (isTerminal) {
				break
			}
		}
	} finally {
		channel.close()
	}
}

function* handleRequestCommand(action: {
	type: string
	payload: { deviceId: string; type: string; params?: Record<string, unknown> }
}) {
	const { deviceId, type, params } = action.payload

	try {
		const uid: string | undefined = yield select((state: RootState) => state.auth.user?.uid)

		if (!uid) {
			yield put(commandError({ deviceId, type, message: 'Not signed in' }))
			return
		}

		const commandId: string = yield call([rtdbService, rtdbService.sendCommand], {
			deviceId,
			uid,
			type,
			params,
		})

		yield put(commandStarted({ deviceId, type, commandId }))
		yield call(watchCommandStatus, deviceId, commandId)
	} catch (err) {
		yield put(
			commandError({
				deviceId,
				type,
				message: err instanceof Error ? err.message : 'Failed to send command',
			})
		)
		yield put(
			addSystemNotification({
				type: 'error',
				title: "Couldn't send command",
				message: 'Check your connection and try again.',
			})
		)
	}
}

export function* commandsSaga() {
	yield takeEvery(COMMAND_ACTIONS.REQUEST_COMMAND, handleRequestCommand)
}
