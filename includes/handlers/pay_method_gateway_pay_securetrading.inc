<?php

/**
 * @file 
 */

class pay_method_gateway_securetrading extends pay_method_direct {
  var $merchant = '';
  var $callbackref = 1;
  var $merchantemail = '';
  var $customeremail = TRUE;
  var $currency = 'gbp';
  var $method_title = 'Credit card (processed by SecureTrading)';
  var $checkout_button = 'Submit order';
  var $confirmation_text = '<p>Check the amounts are correct and press submit to process payment.</p>';


  // Interrupt processing and print out the Secure Trading form.
  function execute($transaction) {
    // Save the transaction as we will exit below.
    $transaction->save();

    // Here we jump out of the form processing flow, and print a form which will be submitted to Secure Trading.
    $return = drupal_get_form('securetrading_form', $transaction, $this);
    print theme('page', $return);
    drupal_page_footer();
    exit();
  }


  function gateway_response($result) {
    //if (function_exists('dsm')) dsm($result);

    // Load the transaction
    $transaction = pay_transaction_load($result['orderref']);

    // If the payment failed, print out the form again.
    if ($result['stresult'] != 1) {
      $this->set_total($transaction->total);
      drupal_set_message(t('The transaction was declined. Please try again.'));
      $return = drupal_get_form('securetrading_form', $transaction, $this);
      print theme('page', $return);
      drupal_page_footer();
      exit();
    }

    // Process transaction as completed. Would be nice if this were a single function call in pay.
    $transaction = pay_transaction_load($result['orderref']);
    $transaction->set_total_paid($result['amount']);
    $activity = $transaction->add_activity($this, $this->pay_form_activity);
    $transaction->update_status($activity->timestamp, TRUE);
    // TODO add the total goal hook. Load the pay_form?

    // Rebuild the cached form.
    $form = form_get_cache($result['drupalformbuildid'], $form_state_store);
    foreach ($form_state_store['storage'] as $key => $value) $form_state[$key] = $value;
    $form['#post'] = $form_state['post'];

    // Unset the pay method which has now been processed.
    // TODO only unset the one instead of clobbering all methods.
    unset($form['#pay']);

    // Jump back into the form flow. This should process the form as if we never left during execute.
    drupal_process_form($form['form_id']['#value'], &$form, &$form_state);
  }

  
  function post_fields($transaction) {
    return array(
      'merchant' => $this->merchant,
      'merchantemail' => $this->merchantemail,
      'customeremail' => $this->customeremail,
      'orderref' => $transaction->pxid,
      'amount' => $this->total() * 100,
      'callbackurl' => $this->callbackref,
      'failureurl' => $this->callbackref,
      'formref' => $this->callbackref,
      'drupalformbuildid' => $this->build_id(),
    );
  }


  // Save a copy of the form to the cache so that we can rebuild it after the Secure Trading response.
  function form(&$form, &$form_state) {
    $build_id = 'securetrading-'. $form['#build_id'];
    $this->build_id($build_id);
    foreach ($form_state as $key => $value) {
      $form_state_store['storage'][$key] = $value;
    }

    form_set_cache($build_id, $form, $form_state_store);
  }


  function settings_form(&$form, &$form_state) {
    parent::settings_form($form, $form_state);

    $group = $this->handler();

    $form[$group]['merchant'] = array(
      '#type' => 'textfield',
      '#title' => t('SecureTrading site reference'),
      '#default_value' => $this->merchant,
      '#size' => 20,
    );
    $form[$group]['callbackref'] = array(
      '#type' => 'select',
      '#title' => t('Template and callback set reference'),
      '#description' => t('SecureTrading allows multiple sets of templates to be uploaded. If unsure, use 1.'),
      '#options' => array(1 => 1, 2, 3, 4, 5, 6, 7, 8),
      '#default_value' => $this->callbackref,
    );
    $form[$group]['merchantemail'] = array(
      '#type' => 'textfield',
      '#title' => t('Merchant email address'),
      '#description' => t('SecureTrading merchant emails will be sent to this address. Leave blank if you do not want to receive these.'),
      '#default_value' => $this->merchantemail,
      '#size' => 40,
    );
    $form[$group]['currency'] = array(
      '#type' => 'textfield',
      '#title' => t('Secure Trade currency code'),
      '#description' => t('See Secure Trade documentation for valid options. Default to gbp for UK pounds.'),
      '#default_value' => $this->currency,
      '#size' => 40,
    );
    $form[$group]['customeremail'] = array(
      '#type' => 'checkbox',
      '#title' => t('Send SecureTrading confirmation emails to customers.'),
      '#default_value' => $this->customeremail,
    );
    $form[$group]['method_title'] = array(
      '#type' => 'textfield',
      '#title' => t('Payment method title'),
      '#default_value' => $this->method_title,
    );
    $form[$group]['confirmation_text'] = array(
      '#type' => 'textarea',
      '#title' => t('Confirmation text displayed to user before submitting form to Secure Trade.'),
      '#description' => t('Provide SecureTrading specific text for the submit button on the order review page.'),
      '#default_value' => $this->confirmation_text,
    );
    $form[$group]['checkout_button'] = array(
      '#type' => 'textfield',
      '#title' => t('Order review submit button text'),
      '#description' => t('Provide SecureTrading specific text for the submit button on the order review page.'),
      '#default_value' => $this->checkout_button,
    );

    $ref = ($this->callbackref == 1)? '':$this->callbackref;

    $callback = "method$ref\tPOST\n";
    $callback .= "url$ref\t". url('pay/gateway/response/securetrading', array('absolute' => TRUE)) ."\n";
    $callback .= "fields$ref\torderref, amount, email, name, address, town, county, country, postcode, telephone, stresult, streference, drupalformbuildid\n";
    $callback .= "pipe$ref\tyes\n";

    $info = '<p>'. t('The following files must be uploaded to SecureTrading for this payment module to function correctly:') .'</p><dl>';
    $info .= '<dt><strong>form'. $ref .'.html</strong></dt>';
    $info .= '<dd>'. t('The form where customers enter their credit card details.') .'</dd>';

    if ($this->customeremail) {
      $info .= '<dt><strong>customeremail'. $ref .'.txt</strong></dt>';
      $info .= '<dd>'. t('Email sent to customers after a successful transaction.') .'</dd>';
    }

    if ($this->merchantemail) {
      $info .= '<dt><strong>merchantemail'. $ref .'.txt</strong></dt>';
      $info .= '<dd>'. t('Email sent to the merchant after a successful transaction.') .'</dd>';

      $info .= '<dt><strong>failureemail'. $ref .'.txt</strong></dt>';
      $info .= '<dd>'. t('Email sent to the merchant after a failed transaction.') .'</dd>';
    }

    $info .= '<dt><strong>callback.txt</strong></dt>';
    $info .= '<dd>'. t('Data file for successful callbacks.') .'</dd>';
    $info .= '<dt><strong>callback-f.txt</strong></dt>';
    $info .= '<dd>'. t('Data file for failed callbacks.') .'</dd>';
    $info .= '</dl>';

    $info .= '<p>'. t('Both callback files must contain the following settings:') .'</p>';
    $info .= '<textarea style="width:90%; height:8em;">'. $callback .'</textarea></li>';

    $form[$group]['general'] = array(
      '#type' => 'fieldset',
      '#title' => t('SecureTrading template and callback files'),
    );
    $form[$group]['general']['info'] = array('#value' => $info);

  }

  function currency_symbol() {
    $map = array(
      'gbp' => '£',
      'usd' => '$',
      'eur' => '€',
    );
    return $map[strtolower($this->currency)];
  }

  // Store the build id for the form.
  function build_id($build_id = NULL) {
    static $_build_id;
    if ($build_id) $_build_id = $build_id;
    return $_build_id;
  }

}
