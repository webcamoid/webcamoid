/* Webcamoid, webcam capture application.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

package org.webcamoid.webcamoidutils;

import java.lang.Integer;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import android.app.Activity;
import android.content.res.Configuration;
import android.view.Gravity;
import android.view.ViewTreeObserver;
import android.view.WindowMetrics;
import android.widget.FrameLayout;
import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.AdSize;
import com.google.android.gms.ads.AdView;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.OnUserEarnedRewardListener;
import com.google.android.gms.ads.appopen.AppOpenAd;
import com.google.android.gms.ads.initialization.InitializationStatus;
import com.google.android.gms.ads.initialization.OnInitializationCompleteListener;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;
import com.google.android.gms.ads.rewarded.RewardItem;
import com.google.android.gms.ads.rewarded.RewardedAd;
import com.google.android.gms.ads.rewarded.RewardedAdLoadCallback;
import com.google.android.gms.ads.rewardedinterstitial.RewardedInterstitialAd;
import com.google.android.gms.ads.rewardedinterstitial.RewardedInterstitialAdLoadCallback;

public class AdManager extends FullScreenContentCallback
{
    // ADTYPE
    public static final int ADTYPE_BANNER                = 0;
    public static final int ADTYPE_ADAPTIVE_BANNER       = 1;
    public static final int ADTYPE_APPOPEN               = 2;
    public static final int ADTYPE_INTERSTITIAL          = 3;
    public static final int ADTYPE_REWARDED              = 4;
    public static final int ADTYPE_REWARDED_INTERSTITIAL = 5;

    // ADCOMPLETIONSTATUS
    public static final int ADCOMPLETIONSTATUS_FAILED    = -1;
    public static final int ADCOMPLETIONSTATUS_SHOWED    =  0;
    public static final int ADCOMPLETIONSTATUS_CLICKED   =  1;
    public static final int ADCOMPLETIONSTATUS_IMPRESSED =  2;
    public static final int ADCOMPLETIONSTATUS_DISMISSED =  3;

    // Public API

    public AdManager(long userPtr,
                     Activity activity,
                     HashMap<Integer, String> adUnitIDMap)
    {
        this.userPtr = userPtr;
        this.activity = activity;
        this.adUnitIDMap = adUnitIDMap;
        this.adsMap = new HashMap<Integer, Object>();
    }

    public boolean initialize()
    {
        MobileAds.initialize(this.activity,
                             new OnInitializationCompleteListener() {
            @Override
            public void onInitializationComplete(InitializationStatus initializationStatus) {
            }
        });

        this.adContainerView = this.activity
                                   .getWindow()
                                   .getDecorView()
                                   .findViewById(android.R.id.content);

        if (this.adContainerView == null)
            return false;

        this.adContainerView.getViewTreeObserver()
                            .addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                long threadId = Thread.currentThread().getId();

                if (!isInitializedOnce) {
                    isInitializedOnce = true;
                    initialized();
                }

                if (!isInitialized) {
                    isInitialized = true;
                    layoutChanged();
                }
            }
        });

        this.activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                adContainerView.requestLayout();
            }
        });

        return true;
    }

    public boolean show(int adType)
    {
        switch (adType) {
        case AdManager.ADTYPE_BANNER:
        case AdManager.ADTYPE_ADAPTIVE_BANNER:
            return this.showBanner(adType);

        case AdManager.ADTYPE_APPOPEN:
            return this.showAppOpen();

        case AdManager.ADTYPE_INTERSTITIAL:
            return this.showInterstitial();

        case AdManager.ADTYPE_REWARDED:
            return this.showRewarded();

        case AdManager.ADTYPE_REWARDED_INTERSTITIAL:
            return this.showRewardedInterstitial();

        default:
            break;
        }

        return false;
    }

    public AdSize bannerSize()
    {
        float adWidthPixels = this.adContainerView.getWidth();

        if (adWidthPixels == 0f)
            adWidthPixels = this.activity
                                .getWindow()
                                .getDecorView()
                                .getWidth();

        float density = this.activity
                            .getResources()
                            .getDisplayMetrics()
                            .density;
        int adWidth = (int) (adWidthPixels / density);

        return AdSize.getCurrentOrientationAnchoredAdaptiveBannerAdSize(this.activity,
                                                                        adWidth);
    }

    // Callbacks

    private void initialized()
    {
        this.show(AdManager.ADTYPE_APPOPEN);
        //this.load(AdManager.ADTYPE_INTERSTITIAL);
        //this.load(AdManager.ADTYPE_REWARDED);
        //this.load(AdManager.ADTYPE_REWARDED_INTERSTITIAL);
    }

    private void layoutChanged()
    {
        this.show(AdManager.ADTYPE_ADAPTIVE_BANNER);
    }

    private void loaded(int adType)
    {
    }

    private void completed(int adType, int status)
    {
    }

    private void rewardEarned(int amount, String type)
    {
    }

    public void bannerSizeChanged(AdSize size)
    {
        int width = size.getWidthInPixels(this.activity);
        int height = size.getHeightInPixels(this.activity);

        adBannerSizeChanged(this.userPtr, width, height);
    }

    // Private API

    private long userPtr = 0;
    private Activity activity;
    private HashMap<Integer, String> adUnitIDMap;
    private HashMap<Integer, Object> adsMap;
    private FrameLayout adContainerView;
    private boolean isInitialized = false;
    private static boolean isInitializedOnce = false;
    private boolean isAdLoading = false;
    private boolean isAdShowing = false;
    private long loadTime = 0;

    private boolean isAdAvailable(int adType)
    {
        return this.adsMap.get(Integer.valueOf(adType)) != null;
    }

    private boolean load(int adType)
    {
        return this.load(adType, false);
    }

    private boolean load(int adType, boolean showAd)
    {
        switch (adType) {
        case AdManager.ADTYPE_BANNER:
        case AdManager.ADTYPE_ADAPTIVE_BANNER:
            return this.loadBanner(adType, showAd);

        case AdManager.ADTYPE_APPOPEN:
            return this.loadAppOpen(showAd);

        case AdManager.ADTYPE_INTERSTITIAL:
            return this.loadInterstitial(showAd);

        case AdManager.ADTYPE_REWARDED:
            return this.loadRewarded(showAd);

        case AdManager.ADTYPE_REWARDED_INTERSTITIAL:
            return this.loadRewardedInterstitial(showAd);

        default:
            break;
        }

        return false;
    }

    private boolean loadBanner(int adType, boolean showAd)
    {
        AdView ad = new AdView(this.activity);

        if (ad == null)
            return false;

        this.adsMap.put(Integer.valueOf(adType), ad);
        ad.setLayoutParams(new FrameLayout.LayoutParams(
                           FrameLayout.LayoutParams.WRAP_CONTENT,
                           FrameLayout.LayoutParams.WRAP_CONTENT,
                           Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM));
        this.adContainerView.addView(ad);
        this.loaded(adType);

        if (showAd)
            this.show(adType);

        return true;
    }

    private boolean loadAppOpen(final boolean showAd)
    {
        final int adType = AdManager.ADTYPE_APPOPEN;

        if (this.isAdLoading || this.isAdAvailable(adType))
            return false;

        if (!this.adUnitIDMap.containsKey(adType))
            return false;

        this.isAdLoading = true;
        AppOpenAd.load(this.activity,
                       this.adUnitIDMap.get(adType),
                       new AdRequest.Builder().build(),
                       new AppOpenAd.AppOpenAdLoadCallback() {
                           @Override
                           public void onAdLoaded(AppOpenAd ad) {
                               adsMap.put(Integer.valueOf(adType), ad);
                               isAdLoading = false;
                               loadTime = (new Date()).getTime();
                               loaded(adType);

                               if (showAd)
                                   show(adType);
                           }

                           @Override
                           public void onAdFailedToLoad(LoadAdError loadAdError) {
                               adsMap.put(Integer.valueOf(adType), null);
                               isAdLoading = false;
                           }
                       });

        return true;
    }

    private boolean loadInterstitial(final boolean showAd)
    {
        final int adType = AdManager.ADTYPE_INTERSTITIAL;

        if (this.isAdLoading || this.isAdAvailable(adType))
            return false;

        if (!this.adUnitIDMap.containsKey(adType))
            return false;

        this.isAdLoading = true;
        InterstitialAd.load(this.activity,
                            this.adUnitIDMap.get(adType),
                            new AdRequest.Builder().build(),
                            new InterstitialAdLoadCallback() {
                                @Override
                                public void onAdLoaded(InterstitialAd ad) {
                                    adsMap.put(Integer.valueOf(adType), ad);
                                    isAdLoading = false;
                                    loadTime = (new Date()).getTime();
                                    loaded(adType);

                                    if (showAd)
                                        show(adType);
                                }

                                @Override
                                public void onAdFailedToLoad(LoadAdError loadAdError) {
                                    adsMap.put(Integer.valueOf(adType), null);
                                    isAdLoading = false;
                                }
                            });

        return true;
    }

    private boolean loadRewarded(final boolean showAd)
    {
        final int adType = AdManager.ADTYPE_REWARDED;

        if (this.isAdLoading || this.isAdAvailable(adType))
            return false;

        if (!this.adUnitIDMap.containsKey(adType))
            return false;

        this.isAdLoading = true;
        RewardedAd.load(this.activity,
                        this.adUnitIDMap.get(adType),
                        new AdRequest.Builder().build(),
                        new RewardedAdLoadCallback() {
                            @Override
                            public void onAdLoaded(RewardedAd ad) {
                                adsMap.put(Integer.valueOf(adType), ad);
                                isAdLoading = false;
                                loadTime = (new Date()).getTime();
                                loaded(adType);

                                if (showAd)
                                    show(adType);
                            }

                            @Override
                            public void onAdFailedToLoad(LoadAdError loadAdError) {
                                adsMap.put(Integer.valueOf(adType), null);
                                isAdLoading = false;
                            }
                        });

        return true;
    }

    private boolean loadRewardedInterstitial(final boolean showAd)
    {
        final int adType = AdManager.ADTYPE_REWARDED_INTERSTITIAL;

        if (this.isAdLoading || this.isAdAvailable(adType))
            return false;

        if (!this.adUnitIDMap.containsKey(adType))
            return false;

        this.isAdLoading = true;
        RewardedInterstitialAd.load(this.activity,
                                    this.adUnitIDMap.get(adType),
                                    new AdRequest.Builder().build(),
                                    new RewardedInterstitialAdLoadCallback() {
                                        @Override
                                        public void onAdLoaded(RewardedInterstitialAd ad) {
                                            adsMap.put(Integer.valueOf(adType), ad);
                                            isAdLoading = false;
                                            loadTime = (new Date()).getTime();
                                            loaded(adType);

                                            if (showAd)
                                                show(adType);
                                        }

                                        @Override
                                        public void onAdFailedToLoad(LoadAdError loadAdError) {
                                            adsMap.put(Integer.valueOf(adType), null);
                                            isAdLoading = false;
                                        }
                                     });

        return true;
    }

    private boolean showBanner(int adType)
    {
        if (!this.adUnitIDMap.containsKey(adType))
            return false;

        if (!this.adsMap.containsKey(Integer.valueOf(adType))) {
            this.completed(adType, AdManager.ADCOMPLETIONSTATUS_FAILED);
            this.load(adType, true);

            return false;
        }

        AdView ad = (AdView) this.adsMap.get(Integer.valueOf(adType));
        ad.setAdUnitId(this.adUnitIDMap.get(adType));
        AdSize size = this.bannerSize();
        ad.setAdSize(size);
        ad.loadAd(new AdRequest.Builder().build());
        this.bannerSizeChanged(size);
        this.completed(adType, AdManager.ADCOMPLETIONSTATUS_SHOWED);

        return true;
    }

    private boolean showAppOpen()
    {
        final int adType = AdManager.ADTYPE_APPOPEN;

        if (this.isAdShowing)
            return false;

        if (!this.isAdAvailable(adType)) {
            this.completed(adType, AdManager.ADCOMPLETIONSTATUS_FAILED);
            this.load(adType, true);

            return false;
        }

        AppOpenAd ad = (AppOpenAd) adsMap.get(Integer.valueOf(adType));
        ad.setFullScreenContentCallback(new HandleFullScreen(this, adType));
        this.isAdShowing = true;
        ad.show(this.activity);

        return true;
    }

    private boolean showInterstitial()
    {
        final int adType = AdManager.ADTYPE_INTERSTITIAL;

        if (this.isAdShowing)
            return false;

        if (!this.isAdAvailable(adType)) {
            this.completed(adType, AdManager.ADCOMPLETIONSTATUS_FAILED);
            this.load(adType, true);

            return false;
        }

        InterstitialAd ad = (InterstitialAd) adsMap.get(Integer.valueOf(adType));
        ad.setFullScreenContentCallback(new HandleFullScreen(this, adType));
        this.isAdShowing = true;
        ad.show(this.activity);

        return true;
    }

    private boolean showRewarded()
    {
        final int adType = AdManager.ADTYPE_REWARDED;

        if (this.isAdShowing)
            return false;

        if (!this.isAdAvailable(adType)) {
            this.completed(adType, AdManager.ADCOMPLETIONSTATUS_FAILED);
            this.load(adType, true);

            return false;
        }

        RewardedAd ad = (RewardedAd) adsMap.get(Integer.valueOf(adType));
        ad.setFullScreenContentCallback(new HandleFullScreen(this, adType));
        this.isAdShowing = true;
        ad.show(this.activity, new HandleReward(this));

        return true;
    }

    private boolean showRewardedInterstitial()
    {
        final int adType = AdManager.ADTYPE_REWARDED_INTERSTITIAL;

        if (this.isAdShowing)
            return false;

        if (!this.isAdAvailable(adType)) {
            this.completed(adType, AdManager.ADCOMPLETIONSTATUS_FAILED);
            this.load(adType, true);

            return false;
        }

        RewardedInterstitialAd ad = (RewardedInterstitialAd) adsMap.get(Integer.valueOf(adType));
        ad.setFullScreenContentCallback(new HandleFullScreen(this, adType));
        this.isAdShowing = true;
        ad.show(this.activity, new HandleReward(this));

        return true;
    }

    private class HandleFullScreen extends FullScreenContentCallback
    {
        private AdManager manager;
        private int adType;

        public HandleFullScreen(AdManager manager, int adType)
        {
            this.manager = manager;
            this.adType = adType;
        }

        @Override
        public void onAdShowedFullScreenContent()
        {
            this.manager.completed(this.adType, AdManager.ADCOMPLETIONSTATUS_SHOWED);
        }

        @Override
        public void onAdClicked()
        {
            this.manager.completed(this.adType, AdManager.ADCOMPLETIONSTATUS_CLICKED);
        }

        @Override
        public void onAdImpression()
        {
            this.manager.completed(this.adType, AdManager.ADCOMPLETIONSTATUS_IMPRESSED);
        }

        @Override
        public void onAdDismissedFullScreenContent()
        {
            this.manager.adsMap.put(Integer.valueOf(this.adType), null);
            this.manager.isAdShowing = false;
            this.manager.completed(this.adType, AdManager.ADCOMPLETIONSTATUS_DISMISSED);
            this.manager.load(this.adType);
        }

        @Override
        public void onAdFailedToShowFullScreenContent(AdError adError)
        {
            this.manager.adsMap.put(Integer.valueOf(this.adType), null);
            this.manager.isAdShowing = false;
            this.manager.completed(this.adType, AdManager.ADCOMPLETIONSTATUS_FAILED);
            this.manager.load(this.adType);
        }
    }

    private class HandleReward implements OnUserEarnedRewardListener
    {
        private AdManager manager;

        public HandleReward(AdManager manager)
        {
            this.manager = manager;
        }

        @Override
        public void onUserEarnedReward(RewardItem rewardItem)
        {
            this.manager.rewardEarned(rewardItem.getAmount(),
                                      rewardItem.getType());
        }
    }

    private static native void adBannerSizeChanged(long userPtr,
                                                   int width,
                                                   int height);
    private static native void akLog(String type, String msg);
}
