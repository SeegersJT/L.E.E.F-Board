import { call, put, select, takeLatest } from 'redux-saga/effects'
import {
	PAIRING_ACTIONS,
	verifyCodeLoading,
	verifyCodeSuccess,
	verifyCodeError,
	claimDeviceLoading,
	claimDeviceError,
} from '../actions/Pairing.action'
import { addSystemNotification } from '../actions/Notification.action'
import type { RootState } from '../types/_Root.type'
import { rtdbService, type PairingCodeEntry } from '@/firebase/rtdb.service'

function* handleVerifyCode(action: { type: string; payload: { code: string } }) {
	yield put(verifyCodeLoading(true))

	try {
		const entry: PairingCodeEntry | null = yield call(
			[rtdbService, rtdbService.lookupPairingCode],
			action.payload.code
		)

		if (!entry) {
			yield put(verifyCodeError('invalid'))
			yield put(
				addSystemNotification({
					type: 'error',
					title: 'Code not recognized',
					message: "That code doesn't match any device, or it's expired.",
				})
			)

			return
		}

		yield put(verifyCodeSuccess({ deviceId: entry.deviceId }))
		yield put(
			addSystemNotification({
				type: 'success',
				title: 'Device found',
				message: 'Give it a name to finish pairing.',
			})
		)
	} catch {
		yield put(verifyCodeError('connection'))
		yield put(
			addSystemNotification({
				type: 'error',
				title: "Couldn't verify code",
				message: 'Check your connection and try again.',
			})
		)
	} finally {
		yield put(verifyCodeLoading(false))
	}
}

function* handleClaimDevice(action: {
	type: string
	payload: { code: string; deviceId: string; nickname: string; onSuccess?: () => void }
}) {
	yield put(claimDeviceLoading(true))

	try {
		const uid: string | undefined = yield select((state: RootState) => state.auth.user?.uid)

		if (!uid) {
			yield put(claimDeviceError('connection'))
			yield put(
				addSystemNotification({
					type: 'error',
					title: 'Not signed in',
					message: 'Your session may have expired - sign in again and retry.',
				})
			)
			return
		}

		yield call([rtdbService, rtdbService.claimDevice], { ...action.payload, uid })

		yield put(
			addSystemNotification({
				type: 'success',
				title: 'Device paired',
				message: `${action.payload.nickname} is ready to go.`,
			})
		)
		action.payload.onSuccess?.()
	} catch (err) {
		const errorCode = (err as { code?: string }).code ?? ''
		if (errorCode.toUpperCase().includes('PERMISSION_DENIED')) {
			yield put(claimDeviceError('invalid'))
			yield put(
				addSystemNotification({
					type: 'error',
					title: 'Pairing failed',
					message:
						'That code just expired, or the device was already claimed by someone else.',
				})
			)
		} else {
			yield put(claimDeviceError('connection'))
			yield put(
				addSystemNotification({
					type: 'error',
					title: "Couldn't pair device",
					message: 'Check your connection and try again.',
				})
			)
		}
	} finally {
		yield put(claimDeviceLoading(false))
	}
}

export function* pairingSaga() {
	yield takeLatest(PAIRING_ACTIONS.REQUEST_VERIFY_CODE, handleVerifyCode)
	yield takeLatest(PAIRING_ACTIONS.REQUEST_CLAIM_DEVICE, handleClaimDevice)
}
